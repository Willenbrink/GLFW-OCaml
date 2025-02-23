#include <GLFW/glfw3.h>
#include <string.h>
#include <caml/mlvalues.h>
#include <caml/alloc.h>
#include <caml/memory.h>
#include <caml/fail.h>
#include <caml/callback.h>
#include <caml/bigarray.h>
#include <assert.h>

#ifdef CAMLunused_start /* Introduced in OCaml 4.03 */
# define CAMLvoid CAMLunused_start value unit CAMLunused_end
#else
# define CAMLvoid CAMLunused value unit
#endif

#ifndef Val_none /* These definitions appeared in OCaml 4.12 */
# define Val_none Val_int(0)
# define Some_val(v) Field(v, 0)
# define Tag_some 0
# define Is_none(v) ((v) == Val_none)
# define Is_some(v) Is_block(v)
static value caml_alloc_some(value v)
{
    CAMLparam1(v);
    value some = caml_alloc_small(1, 0);
    Field(some, 0) = v;
    CAMLreturn(some);
}
#endif

/* The use of pointers outside the OCaml heap inside OCaml code was
   deprecated in version 4.11. As per the OCaml 4.12.0 manual in
   section 18.2.3, provided we check such pointers never have their
   lowest bit set, we may represent these as tagged integers. */
static value Val_cptr(void* p)
{
    assert(((uintptr_t)p & 1) == 0);
    return (value)p | 1;
}

#define Cptr_val(t, v) ((t)((v) & ~1))

#define CAML_SETTER_STUB(glfw_setter, name)                             \
    CAMLprim value caml_##glfw_setter(value new_closure)                \
    {                                                                   \
        CAMLparam1(new_closure);                                        \
        CAMLlocal1(previous_closure);                                   \
                                                                        \
        if (name##_closure == Val_unit)                                 \
            previous_closure = Val_none;                                \
        else                                                            \
            previous_closure = caml_alloc_some(name##_closure);         \
        if (Is_none(new_closure))                                       \
        {                                                               \
            if (name##_closure != Val_unit)                             \
            {                                                           \
                glfw_setter(NULL);                                      \
                raise_if_error();                                       \
                caml_remove_generational_global_root(&name##_closure);  \
                name##_closure = Val_unit;                              \
            }                                                           \
        }                                                               \
        else                                                            \
        {                                                               \
            if (name##_closure == Val_unit)                             \
            {                                                           \
                glfw_setter(name##_callback_stub);                      \
                raise_if_error();                                       \
                name##_closure = Some_val(new_closure);                 \
                caml_register_generational_global_root(&name##_closure); \
            }                                                           \
            else                                                        \
                caml_modify_generational_global_root(                   \
                    &name##_closure, Some_val(new_closure));            \
        }                                                               \
        CAMLreturn(previous_closure);                                   \
    }

struct ml_window_callbacks
{
    value window_pos;
    value window_size;
    value window_close;
    value window_refresh;
    value window_focus;
    value window_iconify;
    value window_maximize;
    value framebuffer_size;
    value window_content_scale;
    value key;
    value character;
    value character_mods;
    value mouse_button;
    value cursor_pos;
    value cursor_enter;
    value scroll;
    value drop;
};

#define ML_WINDOW_CALLBACKS_WOSIZE \
    (sizeof(struct ml_window_callbacks) / sizeof(value))

#define CAML_WINDOW_SETTER_STUB(glfw_setter, name)                      \
    CAMLprim value caml_##glfw_setter(value ml_window, value new_closure) \
    {                                                                   \
        CAMLparam1(new_closure);                                        \
        CAMLlocal1(previous_closure);                                   \
        GLFWwindow* window = Cptr_val(GLFWwindow*, ml_window);          \
        struct ml_window_callbacks* ml_window_callbacks =               \
            *(struct ml_window_callbacks**)                             \
            glfwGetWindowUserPointer(window);                           \
                                                                        \
        raise_if_error();                                               \
        if (ml_window_callbacks->name == Val_unit)                      \
            previous_closure = Val_none;                                \
        else                                                            \
            previous_closure = caml_alloc_some(ml_window_callbacks->name); \
        if (Is_none(new_closure))                                       \
        {                                                               \
            if (ml_window_callbacks->name != Val_unit)                  \
            {                                                           \
                glfw_setter(window, NULL);                              \
                ml_window_callbacks->name = Val_unit;                   \
            }                                                           \
        }                                                               \
        else                                                            \
        {                                                               \
            if (ml_window_callbacks->name == Val_unit)                  \
                glfw_setter(window, name##_callback_stub);              \
            caml_modify(&ml_window_callbacks->name, Some_val(new_closure)); \
        }                                                               \
        CAMLreturn(previous_closure);                                   \
    }

enum value_type
{
    Int,
    IntOption,
    ClientApi,
    ContextRobustness,
    OpenGLProfile,
    ContextReleaseBehavior,
    ContextCreationApi,
    String
};

/* All initialization hints are booleans at the moment. */
static const int ml_init_hint[] = {
    GLFW_JOYSTICK_HAT_BUTTONS,
    GLFW_COCOA_CHDIR_RESOURCES,
    GLFW_COCOA_MENUBAR
};

struct ml_window_attrib
{
    int glfw_window_attrib;
    enum value_type value_type;
};

static const struct ml_window_attrib ml_window_attrib[] = {
    {GLFW_FOCUSED, Int},
    {GLFW_ICONIFIED, Int},
    {GLFW_RESIZABLE, Int},
    {GLFW_VISIBLE, Int},
    {GLFW_DECORATED, Int},
    {GLFW_AUTO_ICONIFY, Int},
    {GLFW_FLOATING, Int},
    {GLFW_MAXIMIZED, Int},
    {GLFW_CENTER_CURSOR, Int},
    {GLFW_TRANSPARENT_FRAMEBUFFER, Int},
    {GLFW_HOVERED, Int},
    {GLFW_FOCUS_ON_SHOW, Int},
    {GLFW_RED_BITS, IntOption},
    {GLFW_GREEN_BITS, IntOption},
    {GLFW_BLUE_BITS, IntOption},
    {GLFW_ALPHA_BITS, IntOption},
    {GLFW_DEPTH_BITS, IntOption},
    {GLFW_STENCIL_BITS, IntOption},
    {GLFW_ACCUM_RED_BITS, IntOption},
    {GLFW_ACCUM_GREEN_BITS, IntOption},
    {GLFW_ACCUM_BLUE_BITS, IntOption},
    {GLFW_ACCUM_ALPHA_BITS, IntOption},
    {GLFW_AUX_BUFFERS, IntOption},
    {GLFW_STEREO, Int},
    {GLFW_SAMPLES, IntOption},
    {GLFW_SRGB_CAPABLE, Int},
    {GLFW_REFRESH_RATE, IntOption},
    {GLFW_DOUBLEBUFFER, Int},
    {GLFW_CLIENT_API, ClientApi},
    {GLFW_CONTEXT_VERSION_MAJOR, Int},
    {GLFW_CONTEXT_VERSION_MINOR, Int},
    {GLFW_CONTEXT_REVISION, Int},
    {GLFW_CONTEXT_ROBUSTNESS, ContextRobustness},
    {GLFW_OPENGL_FORWARD_COMPAT, Int},
    {GLFW_OPENGL_DEBUG_CONTEXT, Int},
    {GLFW_OPENGL_PROFILE, OpenGLProfile},
    {GLFW_CONTEXT_RELEASE_BEHAVIOR, ContextReleaseBehavior},
    {GLFW_CONTEXT_NO_ERROR, Int},
    {GLFW_CONTEXT_CREATION_API, ContextCreationApi},
    {GLFW_SCALE_TO_MONITOR, Int},
    {GLFW_COCOA_RETINA_FRAMEBUFFER, Int},
    {GLFW_COCOA_FRAME_NAME, String},
    {GLFW_COCOA_GRAPHICS_SWITCHING, Int},
    {GLFW_X11_CLASS_NAME, String},
    {GLFW_X11_INSTANCE_NAME, String}
};

#include "GLFW_key_conv_arrays.inl"

static inline value caml_list_of_pointer_array(void** array, int count)
{
    CAMLparam0();
    CAMLlocal1(ret);
    ret = Val_emptylist;
    while (count > 0)
    {
        value tmp = caml_alloc_small(2, 0);
        Field(tmp, 0) = Val_cptr(array[--count]);
        Field(tmp, 1) = ret;
        ret = tmp;
    }
    CAMLreturn(ret);
}

static inline value caml_list_of_flags(int flags, int count)
{
    CAMLparam0();
    CAMLlocal1(ret);
    ret = Val_emptylist;
    for (int i = 0; i < count; ++i)
        if (flags >> i & 1)
        {
            value tmp = caml_alloc_small(2, 0);
            Field(tmp, 0) = Val_int(i);
            Field(tmp, 1) = ret;
            ret = tmp;
        }
    CAMLreturn(ret);
}

static inline value caml_copy_vidmode(const GLFWvidmode* vidmode)
{
    value v = caml_alloc_small(6, 0);

    Field(v, 0) = Val_int(vidmode->width);
    Field(v, 1) = Val_int(vidmode->height);
    Field(v, 2) = Val_int(vidmode->redBits);
    Field(v, 3) = Val_int(vidmode->greenBits);
    Field(v, 4) = Val_int(vidmode->blueBits);
    Field(v, 5) = Val_int(vidmode->refreshRate);
    return v;
}

static value error_tag = Val_unit;
static value error_arg = Val_unit;

static void error_callback(int error, const char* description)
{
    switch (error)
    {
    case GLFW_NOT_INITIALIZED:
        error_tag = *caml_named_value("GLFW.NotInitialized");
        break;
    case GLFW_NO_CURRENT_CONTEXT:
        error_tag = *caml_named_value("GLFW.NoCurrentContext");
        break;
    case GLFW_INVALID_ENUM:
        error_tag = *caml_named_value("GLFW.InvalidEnum");
        break;
    case GLFW_INVALID_VALUE:
        error_tag = *caml_named_value("GLFW.InvalidValue");
        break;
    case GLFW_OUT_OF_MEMORY:
        error_tag = *caml_named_value("GLFW.OutOfMemory");
        break;
    case GLFW_API_UNAVAILABLE:
        error_tag = *caml_named_value("GLFW.ApiUnavailable");
        break;
    case GLFW_VERSION_UNAVAILABLE:
        error_tag = *caml_named_value("GLFW.VersionUnavailable");
        break;
    case GLFW_PLATFORM_ERROR:
        error_tag = *caml_named_value("GLFW.PlatformError");
        break;
    case GLFW_FORMAT_UNAVAILABLE:
        error_tag = *caml_named_value("GLFW.FormatUnavailable");
        break;
    case GLFW_NO_WINDOW_CONTEXT:
        error_tag = *caml_named_value("GLFW.NoWindowContext");
    }
    error_arg = caml_copy_string(description);
}

static inline void raise_if_error(void)
{
    if (error_tag != Val_unit)
    {
        value error_tag_dup = error_tag;
        value error_arg_dup = error_arg;

        error_tag = Val_unit;
        error_arg = Val_unit;
        caml_raise_with_arg(error_tag_dup, error_arg_dup);
    }
}

CAMLprim value init_stub(CAMLvoid)
{
    glfwSetErrorCallback(error_callback);
    return Val_unit;
}

CAMLprim value caml_glfwInit(CAMLvoid)
{
    glfwInit();
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwTerminate(CAMLvoid)
{
    glfwTerminate();
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwInitHint(value hint, value ml_val)
{
    const int offset = Int_val(hint);
    /* All updateable attributes are booleans at the moment. */
    const int glfw_val = Bool_val(ml_val);

    glfwInitHint(ml_init_hint[offset], glfw_val);
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwGetVersion(CAMLvoid)
{
    int major, minor, rev;
    value ret;

    glfwGetVersion(&major, &minor, &rev);
    ret = caml_alloc_small(3, 0);
    Field(ret, 0) = Val_int(major);
    Field(ret, 1) = Val_int(minor);
    Field(ret, 2) = Val_int(rev);
    return ret;
}

CAMLprim value caml_glfwGetVersionString(CAMLvoid)
{
    return caml_copy_string(glfwGetVersionString());
}

CAMLprim value caml_glfwGetMonitors(CAMLvoid)
{
    int monitor_count;
    GLFWmonitor** monitors = glfwGetMonitors(&monitor_count);

    raise_if_error();
    return caml_list_of_pointer_array((void**)monitors, monitor_count);
}

CAMLprim value caml_glfwGetPrimaryMonitor(CAMLvoid)
{
    GLFWmonitor* ret = glfwGetPrimaryMonitor();
    raise_if_error();
    return Val_cptr(ret);
}

CAMLprim value caml_glfwGetMonitorPos(value monitor)
{
    int xpos, ypos;
    value ret;

    glfwGetMonitorPos(Cptr_val(GLFWmonitor*, monitor), &xpos, &ypos);
    raise_if_error();
    ret = caml_alloc_small(2, 0);
    Field(ret, 0) = Val_int(xpos);
    Field(ret, 1) = Val_int(ypos);
    return ret;
}

CAMLprim value caml_glfwGetMonitorWorkarea(value monitor)
{
    int xpos, ypos, width, height;
    value ret;

    glfwGetMonitorWorkarea(
        Cptr_val(GLFWmonitor*, monitor), &xpos, &ypos, &width, &height);
    raise_if_error();
    ret = caml_alloc_small(4, 0);
    Field(ret, 0) = Val_int(xpos);
    Field(ret, 1) = Val_int(ypos);
    Field(ret, 2) = Val_int(width);
    Field(ret, 3) = Val_int(height);
    return ret;
}

CAMLprim value caml_glfwGetMonitorPhysicalSize(value monitor)
{
    int width, height;
    value ret;

    glfwGetMonitorPhysicalSize(
        Cptr_val(GLFWmonitor*, monitor), &width, &height);
    raise_if_error();
    ret = caml_alloc_small(2, 0);
    Field(ret, 0) = Val_int(width);
    Field(ret, 1) = Val_int(height);
    return ret;
}

CAMLprim value caml_glfwGetMonitorContentScale(value monitor)
{
    CAMLparam0();
    CAMLlocal3(ml_xscale, ml_yscale, ret);
    float xscale, yscale;

    glfwGetMonitorContentScale(
        Cptr_val(GLFWmonitor*, monitor), &xscale, &yscale);
    raise_if_error();
    ml_xscale = caml_copy_double(xscale);
    ml_yscale = caml_copy_double(yscale);
    ret = caml_alloc_small(2, 0);
    Field(ret, 0) = ml_xscale;
    Field(ret, 1) = ml_yscale;
    CAMLreturn(ret);
}

CAMLprim value caml_glfwGetMonitorName(value monitor)
{
    const char* ret = glfwGetMonitorName(Cptr_val(GLFWmonitor*, monitor));
    raise_if_error();
    return caml_copy_string(ret);
}

static value monitor_closure = Val_unit;

void monitor_callback_stub(GLFWmonitor* monitor, int event)
{
    caml_callback2(
        monitor_closure, Val_cptr(monitor), Val_int(event - GLFW_CONNECTED));
}

CAML_SETTER_STUB(glfwSetMonitorCallback, monitor)

CAMLprim value caml_glfwGetVideoModes(value monitor)
{
    CAMLparam0();
    CAMLlocal2(ret, vm);
    int videomode_count;
    const GLFWvidmode* videomodes =
        glfwGetVideoModes(Cptr_val(GLFWmonitor*, monitor), &videomode_count);

    raise_if_error();
    ret = Val_emptylist;
    while (videomode_count > 0)
    {
        vm = caml_copy_vidmode(videomodes + --videomode_count);
        value tmp = caml_alloc_small(2, 0);
        Field(tmp, 0) = vm;
        Field(tmp, 1) = ret;
        ret = tmp;
    }
    CAMLreturn(ret);
}

CAMLprim value caml_glfwGetVideoMode(value monitor)
{
    const GLFWvidmode* ret = glfwGetVideoMode(Cptr_val(GLFWmonitor*, monitor));
    raise_if_error();
    return caml_copy_vidmode(ret);
}

CAMLprim value caml_glfwSetGamma(value monitor, value gamma)
{
    glfwSetGamma(Cptr_val(GLFWmonitor*, monitor), Double_val(gamma));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwGetGammaRamp(value monitor)
{
    CAMLparam0();
    CAMLlocal1(ret);
    const GLFWgammaramp* gamma_ramp =
        glfwGetGammaRamp(Cptr_val(GLFWmonitor*, monitor));
    raise_if_error();
    const int flags = CAML_BA_UINT16 | CAML_BA_C_LAYOUT;
    intnat size = gamma_ramp->size;
    const unsigned int byte_size = size * sizeof(*gamma_ramp->red);

    ret = caml_alloc_small(3, 0);
    Field(ret, 0) = Val_unit;
    Field(ret, 1) = Val_unit;
    Field(ret, 2) = Val_unit;
    Store_field(ret, 0, caml_ba_alloc(flags, 1, NULL, &size));
    Store_field(ret, 1, caml_ba_alloc(flags, 1, NULL, &size));
    Store_field(ret, 2, caml_ba_alloc(flags, 1, NULL, &size));
    memcpy(Caml_ba_data_val(Field(ret, 0)), gamma_ramp->red, byte_size);
    memcpy(Caml_ba_data_val(Field(ret, 1)), gamma_ramp->green, byte_size);
    memcpy(Caml_ba_data_val(Field(ret, 2)), gamma_ramp->blue, byte_size);
    CAMLreturn(ret);
}

CAMLprim value caml_glfwSetGammaRamp(value monitor, value ml_gamma_ramp)
{
    GLFWgammaramp gamma_ramp;

    gamma_ramp.size =
        caml_ba_num_elts(Caml_ba_array_val(Field(ml_gamma_ramp, 0)));
    gamma_ramp.red = Caml_ba_data_val(Field(ml_gamma_ramp, 0));
    gamma_ramp.green = Caml_ba_data_val(Field(ml_gamma_ramp, 1));
    gamma_ramp.blue = Caml_ba_data_val(Field(ml_gamma_ramp, 2));
    glfwSetGammaRamp(Cptr_val(GLFWmonitor*, monitor), &gamma_ramp);
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwDefaultWindowHints(CAMLvoid)
{
    glfwDefaultWindowHints();
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwWindowHint(value hint, value ml_val)
{
    const int offset = Int_val(hint);
    int glfw_val;

    switch (ml_window_attrib[offset].value_type)
    {
    case Int:
        glfw_val = Int_val(ml_val);
        break;

    case IntOption:
        glfw_val = Is_none(ml_val) ? GLFW_DONT_CARE : Int_val(Some_val(ml_val));
        break;

    case ClientApi:
        if (ml_val == Val_int(0))
            glfw_val = GLFW_NO_API;
        else if (ml_val == Val_int(1))
            glfw_val = GLFW_OPENGL_API;
        else
            glfw_val = GLFW_OPENGL_ES_API;
        break;

    case ContextRobustness:
        if (ml_val == Val_int(0))
            glfw_val = GLFW_NO_ROBUSTNESS;
        else if (ml_val == Val_int(1))
            glfw_val = GLFW_NO_RESET_NOTIFICATION;
        else
            glfw_val = GLFW_LOSE_CONTEXT_ON_RESET;
        break;

    case OpenGLProfile:
        if (ml_val == Val_int(0))
            glfw_val = GLFW_OPENGL_ANY_PROFILE;
        else if (ml_val == Val_int(1))
            glfw_val = GLFW_OPENGL_CORE_PROFILE;
        else
            glfw_val = GLFW_OPENGL_COMPAT_PROFILE;
        break;

    case ContextReleaseBehavior:
        if (ml_val == Val_int(0))
            glfw_val = GLFW_ANY_RELEASE_BEHAVIOR;
        else if (ml_val == Val_int(1))
            glfw_val = GLFW_RELEASE_BEHAVIOR_FLUSH;
        else
            glfw_val = GLFW_RELEASE_BEHAVIOR_NONE;
        break;

    case ContextCreationApi:
        if (ml_val == Val_int(0))
            glfw_val = GLFW_NATIVE_CONTEXT_API;
        else if (ml_val == Val_int(1))
            glfw_val = GLFW_EGL_CONTEXT_API;
        else
            glfw_val = GLFW_OSMESA_CONTEXT_API;
        break;

    case String: /* Special case: need to use glfwWindowHintString. */
        glfwWindowHintString(
            ml_window_attrib[offset].glfw_window_attrib, String_val(ml_val));
        raise_if_error();
        return Val_unit;
    }
    glfwWindowHint(ml_window_attrib[offset].glfw_window_attrib, glfw_val);
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_window_magic(void* window)
{
    window = (void*) ((uintptr_t)window >> 1);

    void* user_pointer = malloc(sizeof(value));
    value callbacks = caml_alloc_small(ML_WINDOW_CALLBACKS_WOSIZE, 0);

    for (unsigned int i = 0; i < ML_WINDOW_CALLBACKS_WOSIZE; ++i)
        Field(callbacks, i) = Val_unit;
    *(value*)user_pointer = callbacks;
    caml_register_generational_global_root(user_pointer);
    glfwSetWindowUserPointer(window, user_pointer);
    return Val_cptr(window);
}

CAMLprim value caml_glfwCreateWindow(
    value width, value height, value title, value mntor, value share, CAMLvoid)
{
    GLFWwindow* window = glfwCreateWindow(
        Int_val(width), Int_val(height), String_val(title),
        Is_none(mntor) ? NULL : Cptr_val(GLFWmonitor*, Some_val(mntor)),
        Is_none(share) ? NULL : Cptr_val(GLFWwindow*, Some_val(share)));
    raise_if_error();
    void* user_pointer = malloc(sizeof(value));
    value callbacks = caml_alloc_small(ML_WINDOW_CALLBACKS_WOSIZE, 0);

    for (unsigned int i = 0; i < ML_WINDOW_CALLBACKS_WOSIZE; ++i)
        Field(callbacks, i) = Val_unit;
    *(value*)user_pointer = callbacks;
    caml_register_generational_global_root(user_pointer);
    glfwSetWindowUserPointer(window, user_pointer);
    return Val_cptr(window);
}

CAMLprim value caml_glfwCreateWindow_byte(value* val_array, int val_count)
{
    (void)val_count;
    return caml_glfwCreateWindow(val_array[0], val_array[1], val_array[2],
                                 val_array[3], val_array[4], Val_unit);
}

CAMLprim value caml_glfwDestroyWindow(value ml_window)
{
    GLFWwindow* window = Cptr_val(GLFWwindow*, ml_window);
    void* user_pointer = glfwGetWindowUserPointer(window);

    raise_if_error();
    caml_remove_generational_global_root(user_pointer);
    free(user_pointer);
    glfwDestroyWindow(window);
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwWindowShouldClose(value window)
{
    int ret = glfwWindowShouldClose(Cptr_val(GLFWwindow*, window));
    raise_if_error();
    return Val_bool(ret);
}

CAMLprim value caml_glfwSetWindowShouldClose(value window, value val)
{
    glfwSetWindowShouldClose(Cptr_val(GLFWwindow*, window), Bool_val(val));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwSetWindowTitle(value window, value title)
{
    glfwSetWindowTitle(Cptr_val(GLFWwindow*, window), String_val(title));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwSetWindowIcon(value window, value images)
{
    unsigned int count = 0;
    value iter = images;
    GLFWimage* glfw_images;

    while (iter != Val_emptylist)
    {
        ++count;
        iter = Field(iter, 1);
    }
    glfw_images = malloc(sizeof(*glfw_images) * count);
    iter = images;
    for (unsigned int i = 0; i < count; ++i)
    {
        value ml_image = Field(iter, 0);
        glfw_images[i].width = Int_val(Field(ml_image, 0));
        glfw_images[i].height = Int_val(Field(ml_image, 1));
        glfw_images[i].pixels = Bytes_val(Field(ml_image, 2));
        iter = Field(iter, 1);
    }
    glfwSetWindowIcon(Cptr_val(GLFWwindow*, window), count, glfw_images);
    free(glfw_images);
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwGetWindowPos(value window)
{
    int xpos, ypos;
    value ret;

    glfwGetWindowPos(Cptr_val(GLFWwindow*, window), &xpos, &ypos);
    raise_if_error();
    ret = caml_alloc_small(2, 0);
    Field(ret, 0) = Val_int(xpos);
    Field(ret, 1) = Val_int(ypos);
    return ret;
}

CAMLprim value caml_glfwSetWindowPos(value window, value xpos, value ypos)
{
    glfwSetWindowPos(
        Cptr_val(GLFWwindow*, window), Int_val(xpos), Int_val(ypos));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwGetWindowSize(value window)
{
    int width, height;
    value ret;

    glfwGetWindowSize(Cptr_val(GLFWwindow*, window), &width, &height);
    raise_if_error();
    ret = caml_alloc_small(2, 0);
    Field(ret, 0) = Val_int(width);
    Field(ret, 1) = Val_int(height);
    return ret;
}

CAMLprim value caml_glfwSetWindowSizeLimits(
    value window, value minW, value minH, value maxW, value maxH)
{
    int glfw_minW = Is_none(minW) ? GLFW_DONT_CARE : Int_val(Some_val(minW));
    int glfw_minH = Is_none(minH) ? GLFW_DONT_CARE : Int_val(Some_val(minH));
    int glfw_maxW = Is_none(maxW) ? GLFW_DONT_CARE : Int_val(Some_val(maxW));
    int glfw_maxH = Is_none(maxH) ? GLFW_DONT_CARE : Int_val(Some_val(maxH));

    glfwSetWindowSizeLimits(Cptr_val(GLFWwindow*, window),
                            glfw_minW, glfw_minH, glfw_maxW, glfw_maxH);
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwSetWindowAspectRatio(value window, value num, value den)
{
    glfwSetWindowAspectRatio(
        Cptr_val(GLFWwindow*, window), Int_val(num), Int_val(den));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwSetWindowSize(value window, value width, value height)
{
    glfwSetWindowSize(
        Cptr_val(GLFWwindow*, window), Int_val(width), Int_val(height));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwGetFramebufferSize(value window)
{
    int width, height;
    value ret;

    glfwGetFramebufferSize(Cptr_val(GLFWwindow*, window), &width, &height);
    raise_if_error();
    ret = caml_alloc_small(2, 0);
    Field(ret, 0) = Val_int(width);
    Field(ret, 1) = Val_int(height);
    return ret;
}

CAMLprim value caml_glfwGetWindowFrameSize(value window)
{
    int left, top, right, bottom;
    value ret;

    glfwGetWindowFrameSize(
        Cptr_val(GLFWwindow*, window), &left, &top, &right, &bottom);
    raise_if_error();
    ret = caml_alloc_small(4, 0);
    Field(ret, 0) = Val_int(left);
    Field(ret, 1) = Val_int(top);
    Field(ret, 2) = Val_int(right);
    Field(ret, 3) = Val_int(bottom);
    return ret;
}

CAMLprim value caml_glfwGetWindowContentScale(value window)
{
    CAMLparam0();
    CAMLlocal3(ml_xscale, ml_yscale, ret);
    float xscale, yscale;

    glfwGetWindowContentScale(Cptr_val(GLFWwindow*, window), &xscale, &yscale);
    raise_if_error();
    ml_xscale = caml_copy_double(xscale);
    ml_yscale = caml_copy_double(yscale);
    ret = caml_alloc_small(2, 0);
    Field(ret, 0) = ml_xscale;
    Field(ret, 1) = ml_yscale;
    CAMLreturn(ret);
}

CAMLprim value caml_glfwGetWindowOpacity(value window)
{
    float opacity = glfwGetWindowOpacity(Cptr_val(GLFWwindow*, window));
    raise_if_error();
    return caml_copy_double(opacity);
}

CAMLprim value caml_glfwSetWindowOpacity(value window, value time)
{
    glfwSetWindowOpacity(Cptr_val(GLFWwindow*, window), Double_val(time));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwIconifyWindow(value window)
{
    glfwIconifyWindow(Cptr_val(GLFWwindow*, window));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwRestoreWindow(value window)
{
    glfwRestoreWindow(Cptr_val(GLFWwindow*, window));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwMaximizeWindow(value window)
{
    glfwMaximizeWindow(Cptr_val(GLFWwindow*, window));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwShowWindow(value window)
{
    glfwShowWindow(Cptr_val(GLFWwindow*, window));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwHideWindow(value window)
{
    glfwHideWindow(Cptr_val(GLFWwindow*, window));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwFocusWindow(value window)
{
    glfwFocusWindow(Cptr_val(GLFWwindow*, window));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwRequestWindowAttention(value window)
{
    glfwRequestWindowAttention(Cptr_val(GLFWwindow*, window));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwGetWindowMonitor(value window)
{
    GLFWmonitor* monitor = glfwGetWindowMonitor(Cptr_val(GLFWwindow*, window));

    raise_if_error();
    return monitor == NULL ? Val_none : caml_alloc_some(Val_cptr(monitor));
}

CAMLprim value caml_glfwSetWindowMonitor(
    value window, value monitor, value xpos, value ypos,
    value width, value height, value refreshRate)
{
    GLFWmonitor* glfw_monitor =
        Is_none(monitor) ? NULL : Cptr_val(GLFWmonitor*, Some_val(monitor));
    int glfw_refresh_rate =
        Is_none(refreshRate) ? GLFW_DONT_CARE : Int_val(Some_val(refreshRate));

    glfwSetWindowMonitor(
        Cptr_val(GLFWwindow*, window), glfw_monitor, Int_val(xpos),
        Int_val(ypos), Int_val(width), Int_val(height), glfw_refresh_rate);
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwSetWindowMonitor_byte(value* val_array, int val_count)
{
    (void)val_count;
    return caml_glfwSetWindowMonitor(
        val_array[0], val_array[1], val_array[2], val_array[3], val_array[4],
        val_array[5], val_array[6]);
}

CAMLprim value caml_glfwGetWindowAttrib(value window, value attribute)
{
    const int offset = Int_val(attribute);
    int glfw_val = glfwGetWindowAttrib(
        Cptr_val(GLFWwindow*, window),
        ml_window_attrib[offset].glfw_window_attrib);
    raise_if_error();
    value ret = Val_unit;

    switch (ml_window_attrib[offset].value_type)
    {
    case Int:
        ret = Val_int(glfw_val);
        break;

    case ClientApi:
        if (glfw_val == GLFW_NO_API)
            ret = Val_int(0);
        else if (glfw_val == GLFW_OPENGL_API)
            ret = Val_int(1);
        else
            ret = Val_int(2);
        break;

    case ContextRobustness:
        if (glfw_val == GLFW_NO_ROBUSTNESS)
            ret = Val_int(0);
        else if (glfw_val == GLFW_NO_RESET_NOTIFICATION)
            ret = Val_int(1);
        else
            ret = Val_int(2);
        break;

    case OpenGLProfile:
        if (glfw_val == GLFW_OPENGL_ANY_PROFILE)
            ret = Val_int(0);
        else if (glfw_val == GLFW_OPENGL_CORE_PROFILE)
            ret = Val_int(1);
        else
            ret = Val_int(2);
        break;

    case ContextCreationApi:
        if (glfw_val == GLFW_NATIVE_CONTEXT_API)
            ret = Val_int(0);
        else if (glfw_val == GLFW_EGL_CONTEXT_API)
            ret = Val_int(1);
        else
            ret = Val_int(2);

    default:;
    }
    return (ret);
}

CAMLprim value caml_glfwSetWindowAttrib(value window, value hint, value ml_val)
{
    const int offset = Int_val(hint);
    /* All updateable attributes are booleans at the moment. */
    const int glfw_val = Int_val(ml_val);

    glfwSetWindowAttrib(Cptr_val(GLFWwindow*, window),
                        ml_window_attrib[offset].glfw_window_attrib, glfw_val);
    raise_if_error();
    return Val_unit;
}

void window_pos_callback_stub(GLFWwindow* window, int xpos, int ypos)
{
    struct ml_window_callbacks* ml_window_callbacks =
        *(struct ml_window_callbacks**)glfwGetWindowUserPointer(window);

    caml_callback3(ml_window_callbacks->window_pos, Val_cptr(window),
                   Val_int(xpos), Val_int(ypos));
}

CAML_WINDOW_SETTER_STUB(glfwSetWindowPosCallback, window_pos)

void window_size_callback_stub(GLFWwindow* window, int width, int height)
{
    struct ml_window_callbacks* ml_window_callbacks =
        *(struct ml_window_callbacks**)glfwGetWindowUserPointer(window);

    caml_callback3(ml_window_callbacks->window_size, Val_cptr(window),
                   Val_int(width), Val_int(height));
}

CAML_WINDOW_SETTER_STUB(glfwSetWindowSizeCallback, window_size)

void window_close_callback_stub(GLFWwindow* window)
{
    struct ml_window_callbacks* ml_window_callbacks =
        *(struct ml_window_callbacks**)glfwGetWindowUserPointer(window);

    caml_callback(ml_window_callbacks->window_close, Val_cptr(window));
}

CAML_WINDOW_SETTER_STUB(glfwSetWindowCloseCallback, window_close)

void window_refresh_callback_stub(GLFWwindow* window)
{
    struct ml_window_callbacks* ml_window_callbacks =
        *(struct ml_window_callbacks**)glfwGetWindowUserPointer(window);

    caml_callback(ml_window_callbacks->window_refresh, Val_cptr(window));
}

CAML_WINDOW_SETTER_STUB(glfwSetWindowRefreshCallback, window_refresh)

void window_focus_callback_stub(GLFWwindow* window, int focused)
{
    struct ml_window_callbacks* ml_window_callbacks =
        *(struct ml_window_callbacks**)glfwGetWindowUserPointer(window);

    caml_callback2(
        ml_window_callbacks->window_focus, Val_cptr(window), Val_bool(focused));
}

CAML_WINDOW_SETTER_STUB(glfwSetWindowFocusCallback, window_focus)

void window_iconify_callback_stub(GLFWwindow* window, int iconified)
{
    struct ml_window_callbacks* ml_window_callbacks =
        *(struct ml_window_callbacks**)glfwGetWindowUserPointer(window);

    caml_callback2(ml_window_callbacks->window_iconify, Val_cptr(window),
                   Val_bool(iconified));
}

CAML_WINDOW_SETTER_STUB(glfwSetWindowIconifyCallback, window_iconify)

void window_maximize_callback_stub(GLFWwindow* window, int maximized)
{
    struct ml_window_callbacks* ml_window_callbacks =
        *(struct ml_window_callbacks**)glfwGetWindowUserPointer(window);

    caml_callback2(ml_window_callbacks->window_maximize, Val_cptr(window),
                   Val_bool(maximized));
}

CAML_WINDOW_SETTER_STUB(glfwSetWindowMaximizeCallback, window_maximize)

void framebuffer_size_callback_stub(GLFWwindow* window, int width, int height)
{
    struct ml_window_callbacks* ml_window_callbacks =
        *(struct ml_window_callbacks**)glfwGetWindowUserPointer(window);

    caml_callback3(ml_window_callbacks->framebuffer_size, Val_cptr(window),
                   Val_int(width), Val_int(height));
}

CAML_WINDOW_SETTER_STUB(glfwSetFramebufferSizeCallback, framebuffer_size)

void window_content_scale_callback_stub(GLFWwindow* window, float xscale,
                                        float yscale)
{
    CAMLparam0();
    CAMLlocal2(ml_xscale, ml_yscale);
    struct ml_window_callbacks* ml_window_callbacks =
        *(struct ml_window_callbacks**)glfwGetWindowUserPointer(window);

    ml_xscale = caml_copy_double(xscale);
    ml_yscale = caml_copy_double(yscale);
    caml_callback3(ml_window_callbacks->window_content_scale, Val_cptr(window),
                   ml_xscale, ml_yscale);
    CAMLreturn0;
}

CAML_WINDOW_SETTER_STUB(glfwSetWindowContentScaleCallback, window_content_scale)

CAMLprim value caml_glfwPollEvents(CAMLvoid)
{
    glfwPollEvents();
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwWaitEvents(CAMLvoid)
{
    glfwWaitEvents();
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwWaitEventsTimeout(value timeout)
{
    glfwWaitEventsTimeout(Double_val(timeout));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwPostEmptyEvent(CAMLvoid)
{
    glfwPostEmptyEvent();
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwGetInputMode(value window, value mode)
{
    int v = glfwGetInputMode(Cptr_val(GLFWwindow*, window),
                             Int_val(mode) + GLFW_CURSOR);

    raise_if_error();
    if (Int_val(mode) == 0)
        return Val_int(v - GLFW_CURSOR_NORMAL);
    return Val_bool(v);
}

CAMLprim value caml_glfwSetInputMode(value window, value mode, value v)
{
    int glfw_val =
        Int_val(mode) == 0 ? Int_val(v) + GLFW_CURSOR_NORMAL : Bool_val(v);

    glfwSetInputMode(
        Cptr_val(GLFWwindow*, window), Int_val(mode) + GLFW_CURSOR, glfw_val);
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwRawMouseMotionSupported(CAMLvoid)
{
    const int ret = glfwRawMouseMotionSupported();
    raise_if_error();
    return Val_bool(ret);
}

CAMLprim value caml_glfwGetKeyName(value key, value scancode)
{
    const char* name =
        glfwGetKeyName(ml_to_glfw_key[Int_val(key)], Int_val(scancode));

    raise_if_error();
    return name == NULL ? Val_none : caml_alloc_some(caml_copy_string(name));
}

CAMLprim value caml_glfwGetKeyScancode(value key)
{
    int ret = glfwGetKeyScancode(ml_to_glfw_key[Int_val(key)]);
    raise_if_error();
    return Val_int(ret);
}

CAMLprim value caml_glfwGetKey(value window, value key)
{
    int ret =
        glfwGetKey(Cptr_val(GLFWwindow*, window), ml_to_glfw_key[Int_val(key)]);
    raise_if_error();
    return Val_bool(ret);
}

CAMLprim value caml_glfwGetMouseButton(value window, value button)
{
    int ret =
        glfwGetMouseButton(Cptr_val(GLFWwindow*, window), Int_val(button));
    raise_if_error();
    return Val_bool(ret);
}

CAMLprim value caml_glfwGetCursorPos(value window)
{
    CAMLparam0();
    CAMLlocal3(ml_xpos, ml_ypos, ret);
    double xpos, ypos;

    glfwGetCursorPos(Cptr_val(GLFWwindow*, window), &xpos, &ypos);
    raise_if_error();
    ml_xpos = caml_copy_double(xpos);
    ml_ypos = caml_copy_double(ypos);
    ret = caml_alloc_small(2, 0);
    Field(ret, 0) = ml_xpos;
    Field(ret, 1) = ml_ypos;
    CAMLreturn(ret);
}

CAMLprim value caml_glfwSetCursorPos(value window, value xpos, value ypos)
{
    glfwSetCursorPos(
        Cptr_val(GLFWwindow*, window), Double_val(xpos), Double_val(ypos));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwCreateCursor(value image, value xhot, value yhot)
{
    GLFWimage glfw_image;
    GLFWcursor* ret;

    glfw_image.width = Int_val(Field(image, 0));
    glfw_image.height = Int_val(Field(image, 1));
    glfw_image.pixels = Bytes_val(Field(image, 2));
    ret = glfwCreateCursor(&glfw_image, Int_val(xhot), Int_val(yhot));
    raise_if_error();
    return Val_cptr(ret);
}

CAMLprim value caml_glfwCreateStandardCursor(value shape)
{
    GLFWcursor* ret =
        glfwCreateStandardCursor(GLFW_ARROW_CURSOR + Int_val(shape));
    raise_if_error();
    return Val_cptr(ret);
}

CAMLprim value caml_glfwDestroyCursor(value cursor)
{
    glfwDestroyCursor(Cptr_val(GLFWcursor*, cursor));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwSetCursor(value window, value cursor)
{
    glfwSetCursor(Cptr_val(GLFWwindow*, window), Cptr_val(GLFWcursor*, cursor));
    raise_if_error();
    return Val_unit;
}

void key_callback_stub(
    GLFWwindow* window, int key, int scancode, int action, int mods)
{
    struct ml_window_callbacks* ml_window_callbacks =
        *(struct ml_window_callbacks**)glfwGetWindowUserPointer(window);
    value args[] = {
        Val_cptr(window), Val_int(glfw_to_ml_key[key - GLFW_KEY_FIRST]),
        Val_int(scancode), Val_int(action), caml_list_of_flags(mods, 4)
    };

    caml_callbackN(
        ml_window_callbacks->key, sizeof(args) / sizeof(*args), args);
}

CAML_WINDOW_SETTER_STUB(glfwSetKeyCallback, key)

void character_callback_stub(GLFWwindow* window, unsigned int codepoint)
{
    struct ml_window_callbacks* ml_window_callbacks =
        *(struct ml_window_callbacks**)glfwGetWindowUserPointer(window);

    caml_callback2(
        ml_window_callbacks->character, Val_cptr(window), Val_int(codepoint));
}

CAML_WINDOW_SETTER_STUB(glfwSetCharCallback, character)

void character_mods_callback_stub(
    GLFWwindow* window, unsigned int codepoint, int mods)
{
    struct ml_window_callbacks* ml_window_callbacks =
        *(struct ml_window_callbacks**)glfwGetWindowUserPointer(window);
    value ml_mods = caml_list_of_flags(mods, 4);

    caml_callback3(ml_window_callbacks->character_mods, Val_cptr(window),
                   Val_int(codepoint), ml_mods);
}

CAML_WINDOW_SETTER_STUB(glfwSetCharModsCallback, character_mods)

void mouse_button_callback_stub(
    GLFWwindow* window, int button, int action, int mods)
{
    struct ml_window_callbacks* ml_window_callbacks =
        *(struct ml_window_callbacks**)glfwGetWindowUserPointer(window);
    value args[] = {
        Val_cptr(window), Val_int(button), Val_bool(action),
        caml_list_of_flags(mods, 4)
    };

    caml_callbackN(
        ml_window_callbacks->mouse_button, sizeof(args) / sizeof(*args), args);
}

CAML_WINDOW_SETTER_STUB(glfwSetMouseButtonCallback, mouse_button)

void cursor_pos_callback_stub(GLFWwindow* window, double xpos, double ypos)
{
    CAMLparam0();
    CAMLlocal2(ml_xpos, ml_ypos);
    struct ml_window_callbacks* ml_window_callbacks =
        *(struct ml_window_callbacks**)glfwGetWindowUserPointer(window);

    ml_xpos = caml_copy_double(xpos);
    ml_ypos = caml_copy_double(ypos);
    caml_callback3(
        ml_window_callbacks->cursor_pos, Val_cptr(window), ml_xpos, ml_ypos);
    CAMLreturn0;
}

CAML_WINDOW_SETTER_STUB(glfwSetCursorPosCallback, cursor_pos)

void cursor_enter_callback_stub(GLFWwindow* window, int entered)
{
    struct ml_window_callbacks* ml_window_callbacks =
        *(struct ml_window_callbacks**)glfwGetWindowUserPointer(window);

    caml_callback2(
        ml_window_callbacks->cursor_enter, Val_cptr(window), Val_bool(entered));
}

CAML_WINDOW_SETTER_STUB(glfwSetCursorEnterCallback, cursor_enter)

void scroll_callback_stub(GLFWwindow* window, double xoffset, double yoffset)
{
    CAMLparam0();
    CAMLlocal2(ml_xoffset, ml_yoffset);
    struct ml_window_callbacks* ml_window_callbacks =
        *(struct ml_window_callbacks**)glfwGetWindowUserPointer(window);

    ml_xoffset = caml_copy_double(xoffset);
    ml_yoffset = caml_copy_double(yoffset);
    caml_callback3(
        ml_window_callbacks->scroll, Val_cptr(window), ml_xoffset, ml_yoffset);
    CAMLreturn0;
}

CAML_WINDOW_SETTER_STUB(glfwSetScrollCallback, scroll)

void drop_callback_stub(GLFWwindow* window, int count, const char** paths)
{
    CAMLparam0();
    CAMLlocal2(ml_paths, str);
    struct ml_window_callbacks* ml_window_callbacks =
        *(struct ml_window_callbacks**)glfwGetWindowUserPointer(window);

    ml_paths = Val_emptylist;
    while (count > 0)
    {
        str = caml_copy_string(paths[--count]);
        value tmp = caml_alloc_small(2, 0);
        Field(tmp, 0) = str;
        Field(tmp, 1) = ml_paths;
        ml_paths = tmp;
    }
    caml_callback2(ml_window_callbacks->drop, Val_cptr(window), ml_paths);
    CAMLreturn0;
}

CAML_WINDOW_SETTER_STUB(glfwSetDropCallback, drop)

CAMLprim value caml_glfwJoystickPresent(value joy)
{
    int ret = glfwJoystickPresent(Int_val(joy));
    raise_if_error();
    return Val_bool(ret);
}

CAMLprim value caml_glfwGetJoystickAxes(value joy)
{
    value ret;
    int count;
    const float* axes = glfwGetJoystickAxes(Int_val(joy), &count);

    raise_if_error();
    ret = caml_alloc_float_array(count);
    for (int i = 0; i < count; ++i)
        Store_double_field(ret, i, axes[i]);
    return ret;
}

CAMLprim value caml_glfwGetJoystickButtons(value joy)
{
    value ret;
    int count;
    const unsigned char* buttons = glfwGetJoystickButtons(Int_val(joy), &count);

    raise_if_error();
    if (count == 0)
        return Atom(0);
    ret = caml_alloc_small(count, 0);
    for (int i = 0; i < count; ++i)
        Field(ret, i) = Val_bool(buttons[i] == GLFW_PRESS);
    return ret;
}

CAMLprim value caml_glfwGetJoystickHats(value joy)
{
    CAMLparam0();
    CAMLlocal1(ret);
    int count;
    const unsigned char* hats = glfwGetJoystickHats(Int_val(joy), &count);

    raise_if_error();
    if (count == 0)
        ret = Atom(0);
    else
    {
        ret = caml_alloc_small(count, 0);
        for (int i = 0; i < count; ++i)
            Field(ret, i) = Val_unit;
        for (int i = 0; i < count; ++i)
            Store_field(ret, i, caml_list_of_flags(hats[i], 4));
    }
    CAMLreturn(ret);
}

CAMLprim value caml_glfwGetJoystickGUID(value joy)
{
    const char* name = glfwGetJoystickGUID(Int_val(joy));

    raise_if_error();
    return name == NULL ? Val_none : caml_alloc_some(caml_copy_string(name));
}

CAMLprim value caml_glfwGetJoystickName(value joy)
{
    const char* name = glfwGetJoystickName(Int_val(joy));

    raise_if_error();
    return name == NULL ? Val_none : caml_alloc_some(caml_copy_string(name));
}

CAMLprim value caml_glfwJoystickIsGamepad(value joy)
{
    int ret = glfwJoystickIsGamepad(Int_val(joy));
    raise_if_error();
    return Val_bool(ret);
}

static value joystick_closure = Val_unit;

void joystick_callback_stub(int joy, int event)
{
    caml_callback2(
        joystick_closure, Val_int(joy), Val_int(event - GLFW_DISCONNECTED));
}

CAML_SETTER_STUB(glfwSetJoystickCallback, joystick)

CAMLprim value caml_glfwUpdateGamepadMappings(value string)
{
    glfwUpdateGamepadMappings(String_val(string));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwGetGamepadName(value joy)
{
    const char* name = glfwGetGamepadName(Int_val(joy));

    raise_if_error();
    return name == NULL ? Val_none : caml_alloc_some(caml_copy_string(name));
}

CAMLprim value caml_glfwGetGamepadState(value joy)
{
    CAMLparam0();
    CAMLlocal3(buttons, axes, ret);
    GLFWgamepadstate gamepad_state;

    glfwGetGamepadState(Int_val(joy), &gamepad_state);
    raise_if_error();
    buttons = caml_alloc_small(15, 0);
    for (unsigned int i = 0; i < 15; ++i)
        Field(buttons, i) = Val_bool(gamepad_state.buttons[i] == GLFW_PRESS);
    axes = caml_alloc_float_array(6);
    for (unsigned int i = 0; i < 6; ++i)
        Store_double_field(axes, i, gamepad_state.axes[i]);
    ret = caml_alloc_small(2, 0);
    Field(ret, 0) = buttons;
    Field(ret, 1) = axes;
    CAMLreturn(ret);
}

CAMLprim value caml_glfwSetClipboardString(CAMLvoid, value string)
{
    glfwSetClipboardString(NULL, String_val(string));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwGetClipboardString(CAMLvoid)
{
    const char* string = glfwGetClipboardString(NULL);
    raise_if_error();
    return caml_copy_string(string);
}

CAMLprim value caml_glfwGetTime(CAMLvoid)
{
    double time = glfwGetTime();
    raise_if_error();
    return caml_copy_double(time);
}

CAMLprim value caml_glfwSetTime(value time)
{
    glfwSetTime(Double_val(time));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwGetTimerValue(CAMLvoid)
{
    uint64_t timer_value = glfwGetTimerValue();
    raise_if_error();
    return caml_copy_int64(timer_value);
}

CAMLprim value caml_glfwGetTimerFrequency(CAMLvoid)
{
    uint64_t timer_frequency = glfwGetTimerFrequency();
    raise_if_error();
    return caml_copy_int64(timer_frequency);
}

CAMLprim value caml_glfwMakeContextCurrent(value window)
{
    glfwMakeContextCurrent(
        Is_none(window) ? NULL : Cptr_val(GLFWwindow*, Some_val(window)));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwGetCurrentContext(CAMLvoid)
{
    GLFWwindow* window = glfwGetCurrentContext();

    raise_if_error();
    return window == NULL ? Val_none : caml_alloc_some(Val_cptr(window));
}

CAMLprim value caml_glfwSwapBuffers(value window)
{
    glfwSwapBuffers(Cptr_val(GLFWwindow*, window));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwSwapInterval(value interval)
{
    glfwSwapInterval(Int_val(interval));
    raise_if_error();
    return Val_unit;
}

CAMLprim value caml_glfwExtensionSupported(value extension)
{
    int result = glfwExtensionSupported(String_val(extension));
    raise_if_error();
    return Val_bool(result);
}
