#pragma once
#include <cstddef>
static_assert(sizeof(void*)==8,"Only Windows x64 IL2CPP is supported");
template<class T> inline T* il2cpp_array_data(void* array){
    return reinterpret_cast<T*>(static_cast<unsigned char*>(array)+0x20);
}
