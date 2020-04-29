#!/usr/bin/python
import ctypes
from ctypes.util import find_library
import os,sys,functools

print 'python version', sys.version_info[0]

if sys.version_info[0] > 2:
    str = str
    unicode = str
    bytes = bytes
    basestring = (str, bytes)
    PYTHON3 = True
    def str_to_bytes(s):
        """Translate string or bytes to bytes.
        """
        if isinstance(s, str):
            return bytes(s, DEFAULT_ENCODING)
        else:
            return s

    def bytes_to_str(b):
        """Translate bytes to string.
        """
        if isinstance(b, bytes):
            return b.decode(DEFAULT_ENCODING)
        else:
            return b
else:
    str = str
    unicode = unicode
    bytes = str
    basestring = basestring
    PYTHON3 = False
    def str_to_bytes(s):
        """Translate string or bytes to bytes.
        """
        if isinstance(s, unicode):
            return s.encode(DEFAULT_ENCODING)
        else:
            return s

    def bytes_to_str(b):
        """Translate bytes to unicode string.
        """
        if isinstance(b, str):
            return unicode(b, DEFAULT_ENCODING)
        else:
            return b

_internal_guard = object()

def find_lib():
    dll = None
    plugin_path = None
    if sys.platform.startswith('linux'):
        p = find_library('vlc')
        try:
            dll = ctypes.CDLL(p)
        except OSError:  # may fail
            dll = ctypes.CDLL('/home/smsd/modules/libvlc.so.5')
    elif sys.platform.startswith('win'):
        # emilon: WORKAROUND! don't try to look for libvlc.dll in silly places, it breaks.
        p = None
        if p is None:
            try:  # some registry settings
                # leaner than win32api, win32con
                if PYTHON3:
                    import winreg as w
                else:
                    import _winreg as w
                for r in w.HKEY_LOCAL_MACHINE, w.HKEY_CURRENT_USER:
                    try:
                        r = w.OpenKey(r, 'Software\\VideoLAN\\VLC')
                        plugin_path, _ = w.QueryValueEx(r, 'InstallDir')
                        w.CloseKey(r)
                        break
                    except w.error:
                        pass
            except ImportError:  # no PyWin32
                pass
            if plugin_path is None:
                 # try some standard locations.
                for p in ('Program Files\\VideoLan\\', 'VideoLan\\',
                          'Program Files\\',           ''):
                    p = 'C:\\' + p + 'VLC\\libvlc.dll'
                    if os.path.exists(p):
                        plugin_path = os.path.dirname(p)
                        break
            if plugin_path is not None:  # try loading
                p = os.getcwd()
                os.chdir(plugin_path)
                 # if chdir failed, this will raise an exception
                dll = ctypes.CDLL('libvlc.dll')
                 # restore cwd after dll has been loaded
                os.chdir(p)
            else:  # may fail
                dll = ctypes.CDLL('libvlc.dll')
        else:
            plugin_path = os.path.dirname(p)
            dll = ctypes.CDLL(p)

    elif sys.platform.startswith('darwin'):
        # FIXME: should find a means to configure path
        d = '/Applications/VLC.app/Contents/MacOS/'
        p = d + 'lib/libvlc.dylib'
        if os.path.exists(p):
            dll = ctypes.CDLL(p)
            d += 'modules'
            if os.path.isdir(d):
                plugin_path = d
        else:  # hope, some PATH is set...
            dll = ctypes.CDLL('libvlc.dylib')

    else:
        raise NotImplementedError('%s: %s not supported' % (sys.argv[0], sys.platform))

    return (dll, plugin_path)

dll, plugin_path  = find_lib()
print plugin_path,dll

try:
    _Ints = (int, long)
except NameError:  # no long in Python 3+
    _Ints =  int
_Seqs = (list, tuple)

_Cfunctions = {}  # from LibVLC __version__
_Globals = globals()  # sys.modules[__name__].__dict__

#print _Cfunctions ,_Globals 

os.environ['VLC_PLUGIN_PATH']='/home/smsd/modules/'
os.environ['LD_LIBRARY_PATH']='/home/smsd/modules/'

class ListPOINTER(object):
    """Just like a POINTER but accept a list of ctype as an argument.
    """
    def __init__(self, etype):
        self.etype = etype

    def from_param(self, param):
        if isinstance(param, _Seqs):
            return (self.etype * len(param))(*param)

def _Cfunction(name, flags, errcheck, *types):
    """(INTERNAL) New ctypes function binding.
    """
    print name , 'xxxx', type
	
    if hasattr(dll, name):
        p = ctypes.CFUNCTYPE(*types)
        f = p((name, dll), flags)
        if errcheck is not None:
            f.errcheck = errcheck
        # replace the Python function
        # in this module, but only when
        # running as python -O or -OO
        if __debug__:
            _Cfunctions[name] = f
        else:
            _Globals[name] = f
        return f
    raise NameError('no function %r' % (name,))

dll = ctypes.CDLL('/home/smsd/modules/libvlc.so.5')
#f = _Cfunction('libvlc_new', ((1,),(1,),), None,ctypes.c_void_p, None)
f=_Cfunction('libvlc_new', ((1,), (1,),), None,
                    ctypes.c_void_p, ctypes.c_int, ListPOINTER(ctypes.c_char_p))
args = ['vlc','-H']
leng = len(args)

#f(leng,args)


dll = ctypes.CDLL('/tmp/nginx.so')
f=_Cfunction('main', ((1,), (1,),), None,
                    ctypes.c_void_p, ctypes.c_int, ListPOINTER(ctypes.c_char_p))
args = ['vlc','-c','/tmp/nginx.conf']
print args
#f(leng,args)
dll = ctypes.CDLL('/usr/lib64/libgstreamer-0.10.so')
f=_Cfunction('gst_element_set_state', ((1,), (1,),), None,
                    ctypes.c_void_p, ctypes.c_int, ListPOINTER(ctypes.c_char_p))
