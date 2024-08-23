#include <iostream>

#include "thread.hpp"
#include "threadworker.hpp"
#include "workflowsglobal.hpp"

LLWFLOWS_NS_USING

int main(int argc, char** argv) {
    Thread::makeMetaObjectForCurrentThread("main");
    std::cout << "main thread id: " << Thread::currentThread().id() << " name: " << Thread::currentThread().name() << std::endl;
    std::cout << "hello world!" << std::endl;
    return 0;
}
