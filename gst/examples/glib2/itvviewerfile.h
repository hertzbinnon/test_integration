#ifndef __ITV_VIEVER_FILE_H__
#define __ITV_VIEVER_FILE_H__

#include <glib-object.h>

G_BEGIN_DECLS
/*
 * Type declaration.
 */
#define ITV_VIEWER_TYPE_FILE itv_viewer_file_get_type ()
G_DECLARE_FINAL_TYPE (ItvViewerFile, itv_viewer_file, ITV_VIEWER, FILE, GObject)

struct _ItvViewerFileClass
{
  GObjectClass parent_class;

  /* Class virtual function fields. */
  void (* open) (ItvViewerFile  *file,
                 GError     **error);

  /* Padding to allow adding up to 12 new virtual functions without
 *    * breaking ABI. */
  gpointer padding[12];
};

/*
 * Method definitions.
 */
ViewerFile *itv_viewer_file_new (void);

G_END_DECLS

#endif
