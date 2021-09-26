#ifndef __JVM_METHOD__
#define __JVM_METHOD__

typedef unsigned short u2;

struct MethodInternal;
struct JVMMethodID
{
    MethodInternal * _method;
public:
    void setMethodNative();
    void * getMethod()const
    {
        return _method;
    }
    void clear_method_counters();
    bool isMethodNative()const;
    bool isMethodStatic()const;
    int getMethodAccessFlags()const;
    void print()const;
    unsigned long native_function_addr()const;
    void native_function_addr(unsigned long v);
    unsigned long signature_handler_addr()const;
    void signature_handler_addr(unsigned long v);
    void set_size_of_parameters(u2 v);
    int get_size_of_parameters()const;
    void assign(const JVMMethodID * method);
    void set_interpreter_entry(unsigned char * entry);
private:
    JVMMethodID(){}//不允许实例化
};

#endif
