/*****************************************************************************
 * android_mediacodec.c: Video decoder module using the Android MediaCodec API
 *****************************************************************************
 * Copyright (C) 2012 Martin Storsjo
 *
 * Authors: Martin Storsjo <martin@martin.st>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <jni.h>
#include <stdint.h>

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_codec.h>
#include <vlc_block_helper.h>
#include <vlc_cpu.h>

#include "../h264_nal.h"
#include "../hevc_nal.h"
#include <OMX_Core.h>
#include <OMX_Component.h>
#include "omxil_utils.h"
#include "android_opaque.h"
#include "../../video_output/android/android_window.h"

#define INFO_OUTPUT_BUFFERS_CHANGED -3
#define INFO_OUTPUT_FORMAT_CHANGED  -2
#define INFO_TRY_AGAIN_LATER        -1

#define THREAD_NAME "android_mediacodec"

extern int jni_attach_thread(JNIEnv **env, const char *thread_name);
extern void jni_detach_thread();
/* JNI functions to get/set an Android Surface object. */
extern jobject jni_LockAndGetAndroidJavaSurface();
extern void jni_UnlockAndroidSurface();
extern void jni_EventHardwareAccelerationError();
extern bool jni_IsVideoPlayerActivityCreated();

/* Implementation of a circular buffer of timestamps with overwriting
 * of older values. MediaCodec has only one type of timestamp, if a
 * block has no PTS, we send the DTS instead. Some hardware decoders
 * cannot cope with this situation and output the frames in the wrong
 * order. As a workaround in this case, we use a FIFO of timestamps in
 * order to remember which input packets had no PTS.  Since an
 * hardware decoder can silently drop frames, this might cause a
 * growing desynchronization with the actual timestamp. Thus the
 * circular buffer has a limited size and will overwrite older values.
 */
typedef struct
{
    uint32_t          begin;
    uint32_t          size;
    uint32_t          capacity;
    int64_t           *buffer;
} timestamp_fifo_t;

static timestamp_fifo_t *timestamp_FifoNew(uint32_t capacity)
{
    timestamp_fifo_t *fifo = calloc(1, sizeof(*fifo));
    if (!fifo)
        return NULL;
    fifo->buffer = malloc(capacity * sizeof(*fifo->buffer));
    if (!fifo->buffer) {
        free(fifo);
        return NULL;
    }
    fifo->capacity = capacity;
    return fifo;
}

static void timestamp_FifoRelease(timestamp_fifo_t *fifo)
{
    free(fifo->buffer);
    free(fifo);
}

static bool timestamp_FifoIsEmpty(timestamp_fifo_t *fifo)
{
    return fifo->size == 0;
}

static bool timestamp_FifoIsFull(timestamp_fifo_t *fifo)
{
    return fifo->size == fifo->capacity;
}

static void timestamp_FifoEmpty(timestamp_fifo_t *fifo)
{
    fifo->size = 0;
}

static void timestamp_FifoPut(timestamp_fifo_t *fifo, int64_t ts)
{
    uint32_t end = (fifo->begin + fifo->size) % fifo->capacity;
    fifo->buffer[end] = ts;
    if (!timestamp_FifoIsFull(fifo))
        fifo->size += 1;
    else
        fifo->begin = (fifo->begin + 1) % fifo->capacity;
}

static int64_t timestamp_FifoGet(timestamp_fifo_t *fifo)
{
    if (timestamp_FifoIsEmpty(fifo))
        return VLC_TS_INVALID;

    int64_t result = fifo->buffer[fifo->begin];
    fifo->begin = (fifo->begin + 1) % fifo->capacity;
    fifo->size -= 1;
    return result;
}

struct decoder_sys_t
{
    jclass media_codec_list_class, media_codec_class, media_format_class;
    jclass buffer_info_class, byte_buffer_class;
    jmethodID tostring;
    jmethodID get_codec_count, get_codec_info_at, is_encoder, get_capabilities_for_type;
    jfieldID profile_levels_field, profile_field, level_field;
    jmethodID get_supported_types, get_name;
    jmethodID create_by_codec_name, configure, start, stop, flush, release;
    jmethodID get_output_format;
    jmethodID get_input_buffers, get_input_buffer;
    jmethodID get_output_buffers, get_output_buffer;
    jmethodID dequeue_input_buffer, dequeue_output_buffer, queue_input_buffer;
    jmethodID release_output_buffer;
    jmethodID create_video_format, set_integer, set_bytebuffer, get_integer;
    jmethodID buffer_info_ctor;
    jmethodID allocate_direct, limit;
    jfieldID size_field, offset_field, pts_field;

    uint32_t nal_size;

    jobject codec;
    jobject buffer_info;
    jobject input_buffers, output_buffers;
    int pixel_format;
    int stride, slice_height;
    char *name;

    bool allocated;
    bool started;
    bool decoded;
    bool error_state;
    bool error_event_sent;

    ArchitectureSpecificCopyData architecture_specific_data;

    /* Direct rendering members. */
    bool direct_rendering;
    picture_t** pp_inflight_pictures; /**< stores the inflight picture for each output buffer or NULL */
    unsigned int i_inflight_pictures;

    timestamp_fifo_t *timestamp_fifo;
};

enum Types
{
    METHOD, STATIC_METHOD, FIELD
};

#define OFF(x) offsetof(struct decoder_sys_t, x)
struct classname
{
    const char *name;
    int offset;
};
static const struct classname classes[] = {
    { "android/media/MediaCodecList", OFF(media_codec_list_class) },
    { "android/media/MediaCodec", OFF(media_codec_class) },
    { "android/media/MediaFormat", OFF(media_format_class) },
    { "android/media/MediaFormat", OFF(media_format_class) },
    { "android/media/MediaCodec$BufferInfo", OFF(buffer_info_class) },
    { "java/nio/ByteBuffer", OFF(byte_buffer_class) },
    { NULL, 0 },
};

struct member
{
    const char *name;
    const char *sig;
    const char *class;
    int offset;
    int type;
    bool critical;
};
static const struct member members[] = {
    { "toString", "()Ljava/lang/String;", "java/lang/Object", OFF(tostring), METHOD, true },

    { "getCodecCount", "()I", "android/media/MediaCodecList", OFF(get_codec_count), STATIC_METHOD, true },
    { "getCodecInfoAt", "(I)Landroid/media/MediaCodecInfo;", "android/media/MediaCodecList", OFF(get_codec_info_at), STATIC_METHOD, true },

    { "isEncoder", "()Z", "android/media/MediaCodecInfo", OFF(is_encoder), METHOD, true },
    { "getSupportedTypes", "()[Ljava/lang/String;", "android/media/MediaCodecInfo", OFF(get_supported_types), METHOD, true },
    { "getName", "()Ljava/lang/String;", "android/media/MediaCodecInfo", OFF(get_name), METHOD, true },
    { "getCapabilitiesForType", "(Ljava/lang/String;)Landroid/media/MediaCodecInfo$CodecCapabilities;", "android/media/MediaCodecInfo", OFF(get_capabilities_for_type), METHOD, true },

    { "profileLevels", "[Landroid/media/MediaCodecInfo$CodecProfileLevel;", "android/media/MediaCodecInfo$CodecCapabilities", OFF(profile_levels_field), FIELD, true },
    { "profile", "I", "android/media/MediaCodecInfo$CodecProfileLevel", OFF(profile_field), FIELD, true },
    { "level", "I", "android/media/MediaCodecInfo$CodecProfileLevel", OFF(level_field), FIELD, true },

    { "createByCodecName", "(Ljava/lang/String;)Landroid/media/MediaCodec;", "android/media/MediaCodec", OFF(create_by_codec_name), STATIC_METHOD, true },
    { "configure", "(Landroid/media/MediaFormat;Landroid/view/Surface;Landroid/media/MediaCrypto;I)V", "android/media/MediaCodec", OFF(configure), METHOD, true },
    { "start", "()V", "android/media/MediaCodec", OFF(start), METHOD, true },
    { "stop", "()V", "android/media/MediaCodec", OFF(stop), METHOD, true },
    { "flush", "()V", "android/media/MediaCodec", OFF(flush), METHOD, true },
    { "release", "()V", "android/media/MediaCodec", OFF(release), METHOD, true },
    { "getOutputFormat", "()Landroid/media/MediaFormat;", "android/media/MediaCodec", OFF(get_output_format), METHOD, true },
    { "getInputBuffers", "()[Ljava/nio/ByteBuffer;", "android/media/MediaCodec", OFF(get_input_buffers), METHOD, false },
    { "getInputBuffer", "(I)Ljava/nio/ByteBuffer;", "android/media/MediaCodec", OFF(get_input_buffer), METHOD, false },
    { "getOutputBuffers", "()[Ljava/nio/ByteBuffer;", "android/media/MediaCodec", OFF(get_output_buffers), METHOD, false },
    { "getOutputBuffer", "(I)Ljava/nio/ByteBuffer;", "android/media/MediaCodec", OFF(get_output_buffer), METHOD, false },
    { "dequeueInputBuffer", "(J)I", "android/media/MediaCodec", OFF(dequeue_input_buffer), METHOD, true },
    { "dequeueOutputBuffer", "(Landroid/media/MediaCodec$BufferInfo;J)I", "android/media/MediaCodec", OFF(dequeue_output_buffer), METHOD, true },
    { "queueInputBuffer", "(IIIJI)V", "android/media/MediaCodec", OFF(queue_input_buffer), METHOD, true },
    { "releaseOutputBuffer", "(IZ)V", "android/media/MediaCodec", OFF(release_output_buffer), METHOD, true },

    { "createVideoFormat", "(Ljava/lang/String;II)Landroid/media/MediaFormat;", "android/media/MediaFormat", OFF(create_video_format), STATIC_METHOD, true },
    { "setInteger", "(Ljava/lang/String;I)V", "android/media/MediaFormat", OFF(set_integer), METHOD, true },
    { "getInteger", "(Ljava/lang/String;)I", "android/media/MediaFormat", OFF(get_integer), METHOD, true },
    { "setByteBuffer", "(Ljava/lang/String;Ljava/nio/ByteBuffer;)V", "android/media/MediaFormat", OFF(set_bytebuffer), METHOD, true },

    { "<init>", "()V", "android/media/MediaCodec$BufferInfo", OFF(buffer_info_ctor), METHOD, true },
    { "size", "I", "android/media/MediaCodec$BufferInfo", OFF(size_field), FIELD, true },
    { "offset", "I", "android/media/MediaCodec$BufferInfo", OFF(offset_field), FIELD, true },
    { "presentationTimeUs", "J", "android/media/MediaCodec$BufferInfo", OFF(pts_field), FIELD, true },

    { "allocateDirect", "(I)Ljava/nio/ByteBuffer;", "java/nio/ByteBuffer", OFF(allocate_direct), STATIC_METHOD, true },
    { "limit", "(I)Ljava/nio/Buffer;", "java/nio/ByteBuffer", OFF(limit), METHOD, true },

    { NULL, NULL, NULL, 0, 0, false },
};

#define GET_INTEGER(obj, name) (*env)->CallIntMethod(env, obj, p_sys->get_integer, (*env)->NewStringUTF(env, name))

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static int  OpenDecoder(vlc_object_t *);
static void CloseDecoder(vlc_object_t *);

static picture_t *DecodeVideo(decoder_t *, block_t **);

static void InvalidateAllPictures(decoder_t *);
static int InsertInflightPicture(decoder_t *, picture_t *, unsigned int );

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
#define DIRECTRENDERING_TEXT N_("Android direct rendering")
#define DIRECTRENDERING_LONGTEXT N_(\
        "Enable Android direct rendering using opaque buffers.")

#define CFG_PREFIX "mediacodec-"

vlc_module_begin ()
    set_description( N_("Video decoder using Android MediaCodec") )
    set_category( CAT_INPUT )
    set_subcategory( SUBCAT_INPUT_VCODEC )
    set_section( N_("Decoding") , NULL )
    set_capability( "decoder", 0 ) /* Only enabled via commandline arguments */
    add_bool(CFG_PREFIX "dr", true,
             DIRECTRENDERING_TEXT, DIRECTRENDERING_LONGTEXT, true)
    set_callbacks( OpenDecoder, CloseDecoder )
vlc_module_end ()

static int jstrcmp(JNIEnv* env, jobject str, const char* str2)
{
    jsize len = (*env)->GetStringUTFLength(env, str);
    if (len != (jsize) strlen(str2))
        return -1;
    const char *ptr = (*env)->GetStringUTFChars(env, str, NULL);
    int ret = memcmp(ptr, str2, len);
    (*env)->ReleaseStringUTFChars(env, str, ptr);
    return ret;
}

static inline bool check_exception( JNIEnv *env )
{
    if ((*env)->ExceptionOccurred(env)) {
        (*env)->ExceptionClear(env);
        return true;
    }
    else
        return false;
}
#define CHECK_EXCEPTION() check_exception( env )

static bool codec_is_blacklisted( const char *p_name, int i_name_len )
{
     static const char *blacklisted_codecs[] = {
        /* software decoders */
        "OMX.google.",
        /* crashes mediaserver */
        "OMX.MTK.VIDEO.DECODER.MPEG4",
        NULL,
     };

     for( const char **pp_bl_codecs = blacklisted_codecs; *pp_bl_codecs != NULL;
          pp_bl_codecs++ )
     {
        if( !strncmp( p_name, *pp_bl_codecs,
            __MIN( strlen(*pp_bl_codecs), i_name_len ) ) )
            return true;
     }
     return false;
}

/*****************************************************************************
 * OpenDecoder: Create the decoder instance
 *****************************************************************************/
static int OpenDecoder(vlc_object_t *p_this)
{
    decoder_t *p_dec = (decoder_t*)p_this;
    decoder_sys_t *p_sys;

    if (p_dec->fmt_in.i_cat != VIDEO_ES && !p_dec->b_force)
        return VLC_EGENERIC;

    const char *mime = NULL;
    switch (p_dec->fmt_in.i_codec) {
    case VLC_CODEC_HEVC: mime = "video/hevc"; break;
    case VLC_CODEC_H264: mime = "video/avc"; break;
    case VLC_CODEC_H263: mime = "video/3gpp"; break;
    case VLC_CODEC_MP4V: mime = "video/mp4v-es"; break;
    case VLC_CODEC_WMV3: mime = "video/x-ms-wmv"; break;
    case VLC_CODEC_VC1:  mime = "video/wvc1"; break;
    case VLC_CODEC_VP8:  mime = "video/x-vnd.on2.vp8"; break;
    case VLC_CODEC_VP9:  mime = "video/x-vnd.on2.vp9"; break;
    default:
        msg_Dbg(p_dec, "codec %4.4s not supported", (char *)&p_dec->fmt_in.i_codec);
        return VLC_EGENERIC;
    }

    size_t fmt_profile = 0;
    if (p_dec->fmt_in.i_codec == VLC_CODEC_H264)
        h264_get_profile_level(&p_dec->fmt_in, &fmt_profile, NULL, NULL);

    /* Allocate the memory needed to store the decoder's structure */
    if ((p_dec->p_sys = p_sys = calloc(1, sizeof(*p_sys))) == NULL)
        return VLC_ENOMEM;

    p_dec->pf_decode_video = DecodeVideo;

    p_dec->fmt_out.i_cat = p_dec->fmt_in.i_cat;
    p_dec->fmt_out.video = p_dec->fmt_in.video;
    p_dec->fmt_out.audio = p_dec->fmt_in.audio;
    p_dec->b_need_packetized = true;

    JNIEnv* env = NULL;
    jni_attach_thread(&env, THREAD_NAME);

    for (int i = 0; classes[i].name; i++) {
        *(jclass*)((uint8_t*)p_sys + classes[i].offset) =
            (*env)->FindClass(env, classes[i].name);

        if (CHECK_EXCEPTION()) {
            msg_Warn(p_dec, "Unable to find class %s", classes[i].name);
            goto error;
        }
    }

    jclass last_class;
    for (int i = 0; members[i].name; i++) {
        if (i == 0 || strcmp(members[i].class, members[i - 1].class))
            last_class = (*env)->FindClass(env, members[i].class);

        if (CHECK_EXCEPTION()) {
            msg_Warn(p_dec, "Unable to find class %s", members[i].class);
            goto error;
        }

        switch (members[i].type) {
        case METHOD:
            *(jmethodID*)((uint8_t*)p_sys + members[i].offset) =
                (*env)->GetMethodID(env, last_class, members[i].name, members[i].sig);
            break;
        case STATIC_METHOD:
            *(jmethodID*)((uint8_t*)p_sys + members[i].offset) =
                (*env)->GetStaticMethodID(env, last_class, members[i].name, members[i].sig);
            break;
        case FIELD:
            *(jfieldID*)((uint8_t*)p_sys + members[i].offset) =
                (*env)->GetFieldID(env, last_class, members[i].name, members[i].sig);
            break;
        }
        if (CHECK_EXCEPTION()) {
            msg_Warn(p_dec, "Unable to find the member %s in %s",
                     members[i].name, members[i].class);
            if (members[i].critical)
                goto error;
        }
    }
    /* getInputBuffers and getOutputBuffers are deprecated if API >= 21
     * use getInputBuffer and getOutputBuffer instead. */
    if (p_sys->get_input_buffer && p_sys->get_output_buffer) {
        p_sys->get_output_buffers =
        p_sys->get_input_buffers = NULL;
    } else if (!p_sys->get_output_buffers && !p_sys->get_input_buffers) {
        msg_Warn(p_dec, "Unable to find get Output/Input Buffer/Buffers");
        goto error;
    }

    int num_codecs = (*env)->CallStaticIntMethod(env, p_sys->media_codec_list_class,
                                                 p_sys->get_codec_count);
    jobject codec_name = NULL;

    for (int i = 0; i < num_codecs; i++) {
        jobject codec_capabilities = NULL;
        jobject profile_levels = NULL;
        jobject info = NULL;
        jobject name = NULL;
        jobject types = NULL;
        jsize name_len = 0;
        int profile_levels_len = 0, num_types = 0;
        const char *name_ptr = NULL;
        bool found = false;

        info = (*env)->CallStaticObjectMethod(env, p_sys->media_codec_list_class,
                                              p_sys->get_codec_info_at, i);
        if ((*env)->CallBooleanMethod(env, info, p_sys->is_encoder))
            goto loopclean;

        codec_capabilities = (*env)->CallObjectMethod(env, info, p_sys->get_capabilities_for_type,
                                                      (*env)->NewStringUTF(env, mime));
        if (CHECK_EXCEPTION()) {
            msg_Warn(p_dec, "Exception occurred in MediaCodecInfo.getCapabilitiesForType");
            goto loopclean;
        } else if (codec_capabilities) {
            profile_levels = (*env)->GetObjectField(env, codec_capabilities, p_sys->profile_levels_field);
            if (profile_levels)
                profile_levels_len = (*env)->GetArrayLength(env, profile_levels);
        }
        msg_Dbg(p_dec, "Number of profile levels: %d", profile_levels_len);

        types = (*env)->CallObjectMethod(env, info, p_sys->get_supported_types);
        num_types = (*env)->GetArrayLength(env, types);
        name = (*env)->CallObjectMethod(env, info, p_sys->get_name);
        name_len = (*env)->GetStringUTFLength(env, name);
        name_ptr = (*env)->GetStringUTFChars(env, name, NULL);
        found = false;

        if (codec_is_blacklisted( name_ptr, name_len))
            goto loopclean;
        for (int j = 0; j < num_types && !found; j++) {
            jobject type = (*env)->GetObjectArrayElement(env, types, j);
            if (!jstrcmp(env, type, mime)) {
                /* The mime type is matching for this component. We
                   now check if the capabilities of the codec is
                   matching the video format. */
                if (p_dec->fmt_in.i_codec == VLC_CODEC_H264 && fmt_profile) {
                    /* This decoder doesn't expose its profiles and is high
                     * profile capable */
                    if (!strncmp(name_ptr, "OMX.LUMEVideoDecoder", __MIN(20, name_len)))
                        found = true;

                    for (int i = 0; i < profile_levels_len && !found; ++i) {
                        jobject profile_level = (*env)->GetObjectArrayElement(env, profile_levels, i);

                        int omx_profile = (*env)->GetIntField(env, profile_level, p_sys->profile_field);
                        size_t codec_profile = convert_omx_to_profile_idc(omx_profile);
                        (*env)->DeleteLocalRef(env, profile_level);
                        if (codec_profile != fmt_profile)
                            continue;
                        /* Some encoders set the level too high, thus we ignore it for the moment.
                           We could try to guess the actual profile based on the resolution. */
                        found = true;
                    }
                }
                else
                    found = true;
            }
            (*env)->DeleteLocalRef(env, type);
        }
        if (found) {
            msg_Dbg(p_dec, "using %.*s", name_len, name_ptr);
            p_sys->name = malloc(name_len + 1);
            memcpy(p_sys->name, name_ptr, name_len);
            p_sys->name[name_len] = '\0';
            codec_name = name;
        }
loopclean:
        if (name)
            (*env)->ReleaseStringUTFChars(env, name, name_ptr);
        if (profile_levels)
            (*env)->DeleteLocalRef(env, profile_levels);
        if (types)
            (*env)->DeleteLocalRef(env, types);
        if (codec_capabilities)
            (*env)->DeleteLocalRef(env, codec_capabilities);
        if (info)
            (*env)->DeleteLocalRef(env, info);
        if (found)
            break;
    }

    if (!codec_name) {
        msg_Dbg(p_dec, "No suitable codec matching %s was found", mime);
        goto error;
    }

    // This method doesn't handle errors nicely, it crashes if the codec isn't found.
    // (The same goes for createDecoderByType.) This is fixed in latest AOSP and in 4.2,
    // but not in 4.1 devices.
    p_sys->codec = (*env)->CallStaticObjectMethod(env, p_sys->media_codec_class,
                                                  p_sys->create_by_codec_name, codec_name);
    if (CHECK_EXCEPTION()) {
        msg_Warn(p_dec, "Exception occurred in MediaCodec.createByCodecName.");
        goto error;
    }
    p_sys->allocated = true;
    p_sys->codec = (*env)->NewGlobalRef(env, p_sys->codec);

    jobject format = (*env)->CallStaticObjectMethod(env, p_sys->media_format_class,
                         p_sys->create_video_format, (*env)->NewStringUTF(env, mime),
                         p_dec->fmt_in.video.i_width, p_dec->fmt_in.video.i_height);

    if (p_dec->fmt_in.i_extra) {
        // Allocate a byte buffer via allocateDirect in java instead of NewDirectByteBuffer,
        // since the latter doesn't allocate storage of its own, and we don't know how long
        // the codec uses the buffer.
        int buf_size = p_dec->fmt_in.i_extra + 20;
        jobject bytebuf = (*env)->CallStaticObjectMethod(env, p_sys->byte_buffer_class,
                                                         p_sys->allocate_direct, buf_size);
        uint32_t size = p_dec->fmt_in.i_extra;
        uint8_t *ptr = (*env)->GetDirectBufferAddress(env, bytebuf);
        if (p_dec->fmt_in.i_codec == VLC_CODEC_H264 && ((uint8_t*)p_dec->fmt_in.p_extra)[0] == 1) {
            convert_sps_pps(p_dec, p_dec->fmt_in.p_extra, p_dec->fmt_in.i_extra,
                            ptr, buf_size,
                            &size, &p_sys->nal_size);
        } else if (p_dec->fmt_in.i_codec == VLC_CODEC_HEVC) {
            convert_hevc_nal_units(p_dec, p_dec->fmt_in.p_extra,
                                   p_dec->fmt_in.i_extra, ptr, buf_size,
                                   &size, &p_sys->nal_size);
        } else {
            memcpy(ptr, p_dec->fmt_in.p_extra, size);
        }
        (*env)->CallObjectMethod(env, bytebuf, p_sys->limit, size);
        (*env)->CallVoidMethod(env, format, p_sys->set_bytebuffer,
                               (*env)->NewStringUTF(env, "csd-0"), bytebuf);
        (*env)->DeleteLocalRef(env, bytebuf);
    }

    /* If the VideoPlayerActivity is not started, MediaCodec opaque
       direct rendering should be disabled since no surface will be
       attached to the JNI. */
    p_sys->direct_rendering = jni_IsVideoPlayerActivityCreated() && var_InheritBool(p_dec, CFG_PREFIX "dr");

    /* There is no way to rotate the video using direct rendering (and using a
     * SurfaceView) before  API 21 (Lollipop). Therefore, we deactivate direct
     * rendering if video doesn't have a normal rotation and if
     * get_input_buffer method is not present (This method exists since API
     * 21). */
    if (p_sys->direct_rendering
        && p_dec->fmt_in.video.orientation != ORIENT_NORMAL
        && !p_sys->get_input_buffer)
        p_sys->direct_rendering = false;

    if (p_sys->direct_rendering) {
        if (p_dec->fmt_in.video.orientation != ORIENT_NORMAL) {
            int i_angle;

            switch (p_dec->fmt_in.video.orientation) {
                case ORIENT_ROTATED_90:
                    i_angle = 90;
                    break;
                case ORIENT_ROTATED_180:
                    i_angle = 180;
                    break;
                case ORIENT_ROTATED_270:
                    i_angle = 270;
                    break;
                default:
                    i_angle = 0;
            }
            (*env)->CallVoidMethod(env, format, p_sys->set_integer,
                                   (*env)->NewStringUTF(env, "rotation-degrees"),
                                   i_angle);
        }

        jobject surf = jni_LockAndGetAndroidJavaSurface();
        if (surf) {
            // Configure MediaCodec with the Android surface.
            (*env)->CallVoidMethod(env, p_sys->codec, p_sys->configure, format, surf, NULL, 0);
            if (CHECK_EXCEPTION()) {
                msg_Warn(p_dec, "Exception occurred in MediaCodec.configure with an output surface.");
                jni_UnlockAndroidSurface();
                goto error;
            }
            p_dec->fmt_out.i_codec = VLC_CODEC_ANDROID_OPAQUE;
        } else {
            msg_Warn(p_dec, "Failed to get the Android Surface, disabling direct rendering.");
            p_sys->direct_rendering = false;
        }
        jni_UnlockAndroidSurface();
    }
    if (!p_sys->direct_rendering) {
        (*env)->CallVoidMethod(env, p_sys->codec, p_sys->configure, format, NULL, NULL, 0);
        if (CHECK_EXCEPTION()) {
            msg_Warn(p_dec, "Exception occurred in MediaCodec.configure");
            goto error;
        }
    }

    (*env)->CallVoidMethod(env, p_sys->codec, p_sys->start);
    if (CHECK_EXCEPTION()) {
        msg_Warn(p_dec, "Exception occurred in MediaCodec.start");
        goto error;
    }
    p_sys->started = true;

    if (p_sys->get_input_buffers && p_sys->get_output_buffers) {
        p_sys->input_buffers = (*env)->CallObjectMethod(env, p_sys->codec, p_sys->get_input_buffers);
        if (CHECK_EXCEPTION()) {
            msg_Err(p_dec, "Exception in MediaCodec.getInputBuffers (OpenDecoder)");
            goto error;
        }
        p_sys->output_buffers = (*env)->CallObjectMethod(env, p_sys->codec, p_sys->get_output_buffers);
        if (CHECK_EXCEPTION()) {
            msg_Err(p_dec, "Exception in MediaCodec.getOutputBuffers (OpenDecoder)");
            goto error;
        }
        p_sys->input_buffers = (*env)->NewGlobalRef(env, p_sys->input_buffers);
        p_sys->output_buffers = (*env)->NewGlobalRef(env, p_sys->output_buffers);
    }
    p_sys->buffer_info = (*env)->NewObject(env, p_sys->buffer_info_class, p_sys->buffer_info_ctor);
    p_sys->buffer_info = (*env)->NewGlobalRef(env, p_sys->buffer_info);
    (*env)->DeleteLocalRef(env, format);

    jni_detach_thread();

    const int timestamp_fifo_size = 32;
    p_sys->timestamp_fifo = timestamp_FifoNew(timestamp_fifo_size);
    if (!p_sys->timestamp_fifo)
        goto error;

    return VLC_SUCCESS;

 error:
    jni_detach_thread();
    CloseDecoder(p_this);
    return VLC_EGENERIC;
}

static void CloseDecoder(vlc_object_t *p_this)
{
    decoder_t *p_dec = (decoder_t *)p_this;
    decoder_sys_t *p_sys = p_dec->p_sys;
    JNIEnv *env = NULL;

    if (!p_sys)
        return;

    /* Invalidate all pictures that are currently in flight in order
     * to prevent the vout from using destroyed output buffers. */
    if (p_sys->direct_rendering)
        InvalidateAllPictures(p_dec);
    jni_attach_thread(&env, THREAD_NAME);
    if (p_sys->input_buffers)
        (*env)->DeleteGlobalRef(env, p_sys->input_buffers);
    if (p_sys->output_buffers)
        (*env)->DeleteGlobalRef(env, p_sys->output_buffers);
    if (p_sys->codec) {
        if (p_sys->started)
        {
            (*env)->CallVoidMethod(env, p_sys->codec, p_sys->stop);
            if (CHECK_EXCEPTION())
                msg_Err(p_dec, "Exception in MediaCodec.stop");
        }
        if (p_sys->allocated)
        {
            (*env)->CallVoidMethod(env, p_sys->codec, p_sys->release);
            if (CHECK_EXCEPTION())
                msg_Err(p_dec, "Exception in MediaCodec.release");
        }
        (*env)->DeleteGlobalRef(env, p_sys->codec);
    }
    if (p_sys->buffer_info)
        (*env)->DeleteGlobalRef(env, p_sys->buffer_info);
    jni_detach_thread();

    free(p_sys->name);
    ArchitectureSpecificCopyHooksDestroy(p_sys->pixel_format, &p_sys->architecture_specific_data);
    free(p_sys->pp_inflight_pictures);
    if (p_sys->timestamp_fifo)
        timestamp_FifoRelease(p_sys->timestamp_fifo);
    free(p_sys);
}

/*****************************************************************************
 * vout callbacks
 *****************************************************************************/
static void UnlockPicture(picture_t* p_pic, bool b_render)
{
    picture_sys_t *p_picsys = p_pic->p_sys;
    decoder_t *p_dec = p_picsys->priv.hw.p_dec;
    decoder_sys_t *p_sys = p_dec->p_sys;

    if (!p_picsys->priv.hw.b_valid)
        return;

    vlc_mutex_lock(get_android_opaque_mutex());

    /* Picture might have been invalidated while waiting on the mutex. */
    if (!p_picsys->priv.hw.b_valid) {
        vlc_mutex_unlock(get_android_opaque_mutex());
        return;
    }

    uint32_t i_index = p_picsys->priv.hw.i_index;
    InsertInflightPicture(p_dec, NULL, i_index);

    /* Release the MediaCodec buffer. */
    JNIEnv *env = NULL;
    jni_attach_thread(&env, THREAD_NAME);
    (*env)->CallVoidMethod(env, p_sys->codec, p_sys->release_output_buffer, i_index, b_render);
    if (CHECK_EXCEPTION())
        msg_Err(p_dec, "Exception in MediaCodec.releaseOutputBuffer (DisplayBuffer)");

    jni_detach_thread();
    p_picsys->priv.hw.b_valid = false;

    vlc_mutex_unlock(get_android_opaque_mutex());
}

static void InvalidateAllPictures(decoder_t *p_dec)
{
    decoder_sys_t *p_sys = p_dec->p_sys;

    vlc_mutex_lock(get_android_opaque_mutex());

    for (unsigned int i = 0; i < p_sys->i_inflight_pictures; ++i) {
        picture_t *p_pic = p_sys->pp_inflight_pictures[i];
        if (p_pic) {
            p_pic->p_sys->priv.hw.b_valid = false;
            p_sys->pp_inflight_pictures[i] = NULL;
        }
    }
    vlc_mutex_unlock(get_android_opaque_mutex());
}

static int InsertInflightPicture(decoder_t *p_dec, picture_t *p_pic,
                                 unsigned int i_index)
{
    decoder_sys_t *p_sys = p_dec->p_sys;

    if (i_index >= p_sys->i_inflight_pictures) {
        picture_t **pp_pics = realloc(p_sys->pp_inflight_pictures,
                                      (i_index + 1) * sizeof (picture_t *));
        if (!pp_pics)
            return -1;
        if (i_index - p_sys->i_inflight_pictures > 0)
            memset(&pp_pics[p_sys->i_inflight_pictures], 0,
                   (i_index - p_sys->i_inflight_pictures) * sizeof (picture_t *));
        p_sys->pp_inflight_pictures = pp_pics;
        p_sys->i_inflight_pictures = i_index + 1;
    }
    p_sys->pp_inflight_pictures[i_index] = p_pic;
    return 0;
}

static void GetOutput(decoder_t *p_dec, JNIEnv *env, picture_t **pp_pic, jlong timeout)
{
    decoder_sys_t *p_sys = p_dec->p_sys;
    while (1) {
        int index = (*env)->CallIntMethod(env, p_sys->codec, p_sys->dequeue_output_buffer,
                                          p_sys->buffer_info, timeout);
        if (CHECK_EXCEPTION()) {
            msg_Err(p_dec, "Exception in MediaCodec.dequeueOutputBuffer (GetOutput)");
            p_sys->error_state = true;
            return;
        }

        if (index >= 0) {
            if (!p_sys->pixel_format) {
                msg_Warn(p_dec, "Buffers returned before output format is set, dropping frame");
                (*env)->CallVoidMethod(env, p_sys->codec, p_sys->release_output_buffer, index, false);
                if (CHECK_EXCEPTION()) {
                    msg_Err(p_dec, "Exception in MediaCodec.releaseOutputBuffer");
                    p_sys->error_state = true;
                    return;
                }
                continue;
            }

            if (!*pp_pic) {
                *pp_pic = decoder_NewPicture(p_dec);
            } else if (p_sys->direct_rendering) {
                picture_t *p_pic = *pp_pic;
                picture_sys_t *p_picsys = p_pic->p_sys;
                int i_prev_index = p_picsys->priv.hw.i_index;
                (*env)->CallVoidMethod(env, p_sys->codec, p_sys->release_output_buffer, i_prev_index, false);
                if (CHECK_EXCEPTION()) {
                    msg_Err(p_dec, "Exception in MediaCodec.releaseOutputBuffer " \
                            "(GetOutput, overwriting previous picture)");
                    p_sys->error_state = true;
                    return;
                }

                // No need to lock here since the previous picture was not sent.
                InsertInflightPicture(p_dec, NULL, i_prev_index);
            }
            if (*pp_pic) {

                picture_t *p_pic = *pp_pic;
                /* If the oldest input block had no PTS, the timestamp
                 * of the frame returned by MediaCodec might be wrong
                 * so we overwrite it with the corresponding dts. */
                int64_t forced_ts = timestamp_FifoGet(p_sys->timestamp_fifo);
                if (forced_ts == VLC_TS_INVALID)
                    p_pic->date = (*env)->GetLongField(env, p_sys->buffer_info, p_sys->pts_field);
                else
                    p_pic->date = forced_ts;

                if (p_sys->direct_rendering) {
                    picture_sys_t *p_picsys = p_pic->p_sys;
                    p_picsys->pf_lock_pic = NULL;
                    p_picsys->pf_unlock_pic = UnlockPicture;
                    p_picsys->priv.hw.p_dec = p_dec;
                    p_picsys->priv.hw.i_index = index;
                    p_picsys->priv.hw.b_valid = true;

                    vlc_mutex_lock(get_android_opaque_mutex());
                    InsertInflightPicture(p_dec, p_pic, index);
                    vlc_mutex_unlock(get_android_opaque_mutex());
                } else {
                    jobject buf;
                    if (p_sys->get_output_buffers)
                        buf = (*env)->GetObjectArrayElement(env, p_sys->output_buffers, index);
                    else
                        buf = (*env)->CallObjectMethod(env, p_sys->codec,
                                                       p_sys->get_output_buffer, index);
                    //jsize buf_size = (*env)->GetDirectBufferCapacity(env, buf);
                    uint8_t *ptr = (*env)->GetDirectBufferAddress(env, buf);

                    //int size = (*env)->GetIntField(env, p_sys->buffer_info, p_sys->size_field);
                    int offset = (*env)->GetIntField(env, p_sys->buffer_info, p_sys->offset_field);
                    ptr += offset; // Check the size parameter as well

                    unsigned int chroma_div;
                    GetVlcChromaSizes(p_dec->fmt_out.i_codec, p_dec->fmt_out.video.i_width,
                                      p_dec->fmt_out.video.i_height, NULL, NULL, &chroma_div);
                    CopyOmxPicture(p_sys->pixel_format, p_pic, p_sys->slice_height, p_sys->stride,
                                   ptr, chroma_div, &p_sys->architecture_specific_data);
                    (*env)->CallVoidMethod(env, p_sys->codec, p_sys->release_output_buffer, index, false);

                    jthrowable exception = (*env)->ExceptionOccurred(env);
                    if (exception != NULL) {
                        jclass illegalStateException = (*env)->FindClass(env, "java/lang/IllegalStateException");
                        if((*env)->IsInstanceOf(env, exception, illegalStateException)) {
                            msg_Err(p_dec, "Codec error (IllegalStateException) in MediaCodec.releaseOutputBuffer");
                            (*env)->ExceptionClear(env);
                            (*env)->DeleteLocalRef(env, illegalStateException);
                            p_sys->error_state = true;
                        }
                    }
                    (*env)->DeleteLocalRef(env, buf);
                }
            } else {
                msg_Warn(p_dec, "NewPicture failed");
                (*env)->CallVoidMethod(env, p_sys->codec, p_sys->release_output_buffer, index, false);
                if (CHECK_EXCEPTION()) {
                    msg_Err(p_dec, "Exception in MediaCodec.releaseOutputBuffer (GetOutput)");
                    p_sys->error_state = true;
                }
            }
            return;

        } else if (index == INFO_OUTPUT_BUFFERS_CHANGED) {
            msg_Dbg(p_dec, "output buffers changed");
            if (!p_sys->get_output_buffers)
                continue;
            (*env)->DeleteGlobalRef(env, p_sys->output_buffers);

            p_sys->output_buffers = (*env)->CallObjectMethod(env, p_sys->codec,
                                                             p_sys->get_output_buffers);
            if (CHECK_EXCEPTION()) {
                msg_Err(p_dec, "Exception in MediaCodec.getOutputBuffer (GetOutput)");
                p_sys->output_buffers = NULL;
                p_sys->error_state = true;
                return;
            }

            p_sys->output_buffers = (*env)->NewGlobalRef(env, p_sys->output_buffers);
        } else if (index == INFO_OUTPUT_FORMAT_CHANGED) {
            jobject format = (*env)->CallObjectMethod(env, p_sys->codec, p_sys->get_output_format);
            if (CHECK_EXCEPTION()) {
                msg_Err(p_dec, "Exception in MediaCodec.getOutputFormat (GetOutput)");
                p_sys->error_state = true;
                return;
            }

            jobject format_string = (*env)->CallObjectMethod(env, format, p_sys->tostring);

            jsize format_len = (*env)->GetStringUTFLength(env, format_string);
            const char *format_ptr = (*env)->GetStringUTFChars(env, format_string, NULL);
            msg_Dbg(p_dec, "output format changed: %.*s", format_len, format_ptr);
            (*env)->ReleaseStringUTFChars(env, format_string, format_ptr);

            ArchitectureSpecificCopyHooksDestroy(p_sys->pixel_format, &p_sys->architecture_specific_data);

            int width           = GET_INTEGER(format, "width");
            int height          = GET_INTEGER(format, "height");
            p_sys->stride       = GET_INTEGER(format, "stride");
            p_sys->slice_height = GET_INTEGER(format, "slice-height");
            p_sys->pixel_format = GET_INTEGER(format, "color-format");
            int crop_left       = GET_INTEGER(format, "crop-left");
            int crop_top        = GET_INTEGER(format, "crop-top");
            int crop_right      = GET_INTEGER(format, "crop-right");
            int crop_bottom     = GET_INTEGER(format, "crop-bottom");

            const char *name = "unknown";
            if (!p_sys->direct_rendering) {
                if (!GetVlcChromaFormat(p_sys->pixel_format,
                                        &p_dec->fmt_out.i_codec, &name)) {
                    msg_Err(p_dec, "color-format not recognized");
                    p_sys->error_state = true;
                    return;
                }
            }

            msg_Err(p_dec, "output: %d %s, %dx%d stride %d %d, crop %d %d %d %d",
                    p_sys->pixel_format, name, width, height, p_sys->stride, p_sys->slice_height,
                    crop_left, crop_top, crop_right, crop_bottom);

            p_dec->fmt_out.video.i_width = crop_right + 1 - crop_left;
            p_dec->fmt_out.video.i_height = crop_bottom + 1 - crop_top;
            if (p_dec->fmt_out.video.i_width <= 1
                || p_dec->fmt_out.video.i_height <= 1) {
                p_dec->fmt_out.video.i_width = width;
                p_dec->fmt_out.video.i_height = height;
            }
            p_dec->fmt_out.video.i_visible_width = p_dec->fmt_out.video.i_width;
            p_dec->fmt_out.video.i_visible_height = p_dec->fmt_out.video.i_height;

            if (p_sys->stride <= 0)
                p_sys->stride = width;
            if (p_sys->slice_height <= 0)
                p_sys->slice_height = height;
            CHECK_EXCEPTION();

            ArchitectureSpecificCopyHooks(p_dec, p_sys->pixel_format, p_sys->slice_height,
                                          p_sys->stride, &p_sys->architecture_specific_data);
            if (p_sys->pixel_format == OMX_TI_COLOR_FormatYUV420PackedSemiPlanar)
                p_sys->slice_height -= crop_top/2;
            if (IgnoreOmxDecoderPadding(p_sys->name)) {
                p_sys->slice_height = 0;
                p_sys->stride = p_dec->fmt_out.video.i_width;
            }

        } else {
            return;
        }
    }
}

static picture_t *DecodeVideo(decoder_t *p_dec, block_t **pp_block)
{
    decoder_sys_t *p_sys = p_dec->p_sys;
    picture_t *p_pic = NULL;
    JNIEnv *env = NULL;
    struct H264ConvertState convert_state = { 0, 0 };

    if (!pp_block || !*pp_block)
        return NULL;

    block_t *p_block = *pp_block;

    if (p_sys->error_state) {
        block_Release(p_block);
        if (!p_sys->error_event_sent) {
            /* Signal the error to the Java. */
            jni_EventHardwareAccelerationError();
            p_sys->error_event_sent = true;
        }
        return NULL;
    }

    jni_attach_thread(&env, THREAD_NAME);

    if (p_block->i_flags & (BLOCK_FLAG_DISCONTINUITY|BLOCK_FLAG_CORRUPTED)) {
        block_Release(p_block);
        timestamp_FifoEmpty(p_sys->timestamp_fifo);
        if (p_sys->decoded) {
            /* Invalidate all pictures that are currently in flight
             * since flushing make all previous indices returned by
             * MediaCodec invalid. */
            if (p_sys->direct_rendering)
                InvalidateAllPictures(p_dec);

            (*env)->CallVoidMethod(env, p_sys->codec, p_sys->flush);
            if (CHECK_EXCEPTION()) {
                msg_Warn(p_dec, "Exception occurred in MediaCodec.flush");
                p_sys->error_state = true;
            }
        }
        p_sys->decoded = false;
        jni_detach_thread();
        return NULL;
    }

    /* Use the aspect ratio provided by the input (ie read from packetizer).
     * Don't check the current value of the aspect ratio in fmt_out, since we
     * want to allow changes in it to propagate. */
    if (p_dec->fmt_in.video.i_sar_num != 0 && p_dec->fmt_in.video.i_sar_den != 0) {
        p_dec->fmt_out.video.i_sar_num = p_dec->fmt_in.video.i_sar_num;
        p_dec->fmt_out.video.i_sar_den = p_dec->fmt_in.video.i_sar_den;
    }

    jlong timeout = 0;
    const int max_polling_attempts = 50;
    int attempts = 0;
    while (true) {
        int index = (*env)->CallIntMethod(env, p_sys->codec, p_sys->dequeue_input_buffer, (jlong) 0);
        if (CHECK_EXCEPTION()) {
            msg_Err(p_dec, "Exception occurred in MediaCodec.dequeueInputBuffer");
            p_sys->error_state = true;
            break;
        }

        if (index < 0) {
            GetOutput(p_dec, env, &p_pic, timeout);
            if (p_sys->error_state)
                break;
            if (p_pic) {
                /* If we couldn't get an available input buffer but a
                 * decoded frame is available, we return the frame
                 * without assigning NULL to *pp_block. The next call
                 * to DecodeVideo will try to send the input packet again.
                 */
                jni_detach_thread();
                return p_pic;
            }
            timeout = 30 * 1000;
            ++attempts;
            /* With opaque DR the output buffers are released by the
               vout therefore we implement a timeout for polling in
               order to avoid being indefinitely stalled in this loop. */
            if (p_sys->direct_rendering && attempts == max_polling_attempts) {
                picture_t *invalid_picture = decoder_NewPicture(p_dec);
                if (invalid_picture) {
                    invalid_picture->date = VLC_TS_INVALID;
                    picture_sys_t *p_picsys = invalid_picture->p_sys;
                    p_picsys->pf_lock_pic = NULL;
                    p_picsys->pf_unlock_pic = NULL;
                    p_picsys->priv.hw.p_dec = NULL;
                    p_picsys->priv.hw.i_index = -1;
                    p_picsys->priv.hw.b_valid = false;
                }
                else {
                    /* If we cannot return a picture we must free the
                       block since the decoder will proceed with the
                       next block. */
                    block_Release(p_block);
                    *pp_block = NULL;
                }
                jni_detach_thread();
                return invalid_picture;
            }
            continue;
        }

        jobject buf;
        if (p_sys->get_input_buffers)
            buf = (*env)->GetObjectArrayElement(env, p_sys->input_buffers, index);
        else
            buf = (*env)->CallObjectMethod(env, p_sys->codec, p_sys->get_input_buffer, index);
        jsize size = (*env)->GetDirectBufferCapacity(env, buf);
        uint8_t *bufptr = (*env)->GetDirectBufferAddress(env, buf);
        if (size < 0) {
            msg_Err(p_dec, "Java buffer has invalid size");
            p_sys->error_state = true;
            break;
        }
        if ((size_t) size > p_block->i_buffer)
            size = p_block->i_buffer;
        memcpy(bufptr, p_block->p_buffer, size);

        convert_h264_to_annexb(bufptr, size, p_sys->nal_size, &convert_state);

        int64_t ts = p_block->i_pts;
        if (!ts && p_block->i_dts)
            ts = p_block->i_dts;
        timestamp_FifoPut(p_sys->timestamp_fifo, p_block->i_pts ? VLC_TS_INVALID : p_block->i_dts);
        (*env)->CallVoidMethod(env, p_sys->codec, p_sys->queue_input_buffer, index, 0, size, ts, 0);
        (*env)->DeleteLocalRef(env, buf);
        if (CHECK_EXCEPTION()) {
            msg_Err(p_dec, "Exception in MediaCodec.queueInputBuffer");
            p_sys->error_state = true;
            break;
        }
        p_sys->decoded = true;
        break;
    }
    if (p_sys->error_state) {
        if (p_pic)
            picture_Release(p_pic);
        jni_detach_thread();
        return NULL;
    }
    if (!p_pic)
        GetOutput(p_dec, env, &p_pic, 0);
    jni_detach_thread();

    block_Release(p_block);
    *pp_block = NULL;

    return p_pic;
}
