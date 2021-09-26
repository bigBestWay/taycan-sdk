#ifndef __JAVA_NATIVE_BASE__
#define __JAVA_NATIVE_BASE__

#include <string>
#include "jni.h"
#include "jvmti.h"

class JVMHelper;
struct JVMMethodID;
class JavaNativeBase
{
public:
    JavaNativeBase();
    virtual ~JavaNativeBase();
    /*
    将方法hook为新地址，新地址的函数声明应该符合原方法签名。比如：
    Java方法声明为void aaa(ServletRequest a,  ServletResponse)
    对应的hook方法声明为void JNICALL hook_internalFilter(JNIEnv * env, jobject thiz, jobject req, jobject rsp)
    第1个参数固定为JNIEnv指针
    第2个参数如果方法为非静态则为this指针，如果是静态就不需要this，依次是函数参数即可
    */
    static int hookJvmMethod(JVMMethodID * method, unsigned long new_method_addr);
    static JNIEnv * getJNIEnv();
    static jvmtiEnv * getJVMTIEnv();
    /*
    将类名签名化，比如
    输入org.apache.catalina.core.ApplicationFilterChain
    输出Lorg/apache/catalina/core/ApplicationFilterChain;
    */
    static std::string to_signature(const char * classname);
    /*
    通过jnienv查找类，由于classLoader的关系很多都查找不到。使用jvmtienv查找即可。
    */
    static jclass jvmti_find_class(const char * classname);
    static jmethodID jvmti_get_method(jclass clazz, const char * methodname, const char * signature);
private:
    static JNIEnv_ * _jni_env;
    static jvmtiEnv * _jvmti_env;
    static JVMHelper * _jvm_helper;
    static unsigned char * _native_interpreter_entry;
};

#endif
