/*
The MIT License
Copyright (c) 2019 Geoffrey Daniels. http://gpdaniels.com/
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE
*/

///////////////////////////////////////////////////////////////////////////////
// Functions required by compilers.
///////////////////////////////////////////////////////////////////////////////

#if defined(__clang__)
    // The function called when a pure virtual function is called.
    extern "C" void __cxa_pure_virtual();
    extern "C" void __cxa_pure_virtual() {
    }

    // The guard variable type.
    __extension__ typedef int __guard __attribute__((mode(__DI__)));

    // Functions for guarding a function scope static variable construction.
    extern "C" int __cxa_guard_acquire(__guard* guard);
    extern "C" int __cxa_guard_acquire (__guard* guard) {
        return !*reinterpret_cast<char*>(guard);
    }
    extern "C" void __cxa_guard_release(__guard* guard);
    extern "C" void __cxa_guard_release(__guard* guard) {
        *reinterpret_cast<char*>(guard) = 1;
    }
    extern "C" void __cxa_guard_abort (__guard* guard);
    extern "C" void __cxa_guard_abort (__guard* guard) {
        static_cast<void>(guard);
    }

#elif (defined(__GNUC__) || defined(__GNUG__)) && !(defined(__clang__) || defined(__INTEL_COMPILER))
    // The function called when a pure virtual function is called.
    extern "C" void __cxa_pure_virtual();
    extern "C" void __cxa_pure_virtual() {
    }

    // The guard variable type.
    __extension__ typedef int __guard __attribute__((mode(__DI__)));

    // Functions for guarding a function scope static variable construction.
    extern "C" int __cxa_guard_acquire(__guard* guard);
    extern "C" int __cxa_guard_acquire (__guard* guard) {
        return !*reinterpret_cast<char*>(guard);
    }
    extern "C" void __cxa_guard_release(__guard* guard);
    extern "C" void __cxa_guard_release(__guard* guard) {
        *reinterpret_cast<char*>(guard) = 1;
    }
    extern "C" void __cxa_guard_abort (__guard* guard);
    extern "C" void __cxa_guard_abort (__guard* guard) {
        static_cast<void>(guard);
    }
#else
    #error "No support for this compiler."
#endif

///////////////////////////////////////////////////////////////////////////////
// Memory allocation function stubs.
///////////////////////////////////////////////////////////////////////////////

void* operator new(__SIZE_TYPE__ size);
void* operator new(__SIZE_TYPE__ size) {
    static_cast<void>(size);
    asm volatile("syscall" : : "a" (60), "D" (-1u));
    return reinterpret_cast<void*>(-1ull);
}

void *operator new[](__SIZE_TYPE__ size);
void *operator new[](__SIZE_TYPE__ size) {
    static_cast<void>(size);
    asm volatile("syscall" : : "a" (60), "D" (-1u));
    return reinterpret_cast<void*>(-1ull);
}

void operator delete(void* memory_pointer) noexcept;
void operator delete(void* memory_pointer) noexcept {
    static_cast<void>(memory_pointer);
    asm volatile("syscall" : : "a" (60), "D" (-1u));
}

void operator delete[](void* memory_pointer) noexcept;
void operator delete[](void* memory_pointer) noexcept {
    static_cast<void>(memory_pointer);
    asm volatile("syscall" : : "a" (60), "D" (-1u));
}

void operator delete(void* memory_pointer, __SIZE_TYPE__ size) noexcept;
void operator delete(void* memory_pointer, __SIZE_TYPE__ size) noexcept {
    static_cast<void>(memory_pointer);
    static_cast<void>(size);
    asm volatile("syscall" : : "a" (60), "D" (-1u));
}

void operator delete[](void* memory_pointer, __SIZE_TYPE__ size) noexcept;
void operator delete[](void* memory_pointer, __SIZE_TYPE__ size) noexcept {
    static_cast<void>(memory_pointer);
    static_cast<void>(size);
    asm volatile("syscall" : : "a" (60), "D" (-1u));
}

///////////////////////////////////////////////////////////////////////////////
// At exit function and associated variables.
///////////////////////////////////////////////////////////////////////////////

using function_pointer_type = void(*)();
constexpr static const __SIZE_TYPE__ atexit_max_functions = 128;
static function_pointer_type atexit_functions[atexit_max_functions];
static __SIZE_TYPE__ atexit_function_count = 0;

extern "C" int atexit(function_pointer_type function);

extern "C" int atexit(function_pointer_type function) {
    if (atexit_function_count >= atexit_max_functions) {
        return -1;
    };
    atexit_functions[atexit_function_count++] = function;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Entry point and setup functions.
///////////////////////////////////////////////////////////////////////////////

// Entry point.
extern "C" [[noreturn, gnu::naked]] void _start();
extern "C" [[noreturn, gnu::naked]] void _start() {
    // This code assumes that there is no frame pointer.
    // If compiled with frame pointers uncomment the below line.
    // asm volatile("pop %rbp");

    // Copy the stack pointer to the frame pointer.
    asm volatile("mov %rsp, %rbp");

    // Read x64 ABI variables for argc, argv and envp.
    // argc = RDI = [ RSP ]
    // argv = RSI = [ RSP+8 ]
    // envp = RDX = [ argv + (8 * argc) + 8 ]
    asm volatile("movq (0)(%rbp), %rdi");
    asm volatile("leaq (8)(%rbp), %rsi");
    asm volatile("leaq (8)(%rsi, %rdi, 8), %rdx");

    // Ensure the stack is aligned.
    asm volatile("andq $-15, %rsp");

    // Call the setup function that calls static constructors, main, and static destructors.
    asm volatile("call _setup");
}

// Forward declaration of the main function.
int main(int argument_count, char* arguments[], char* environment[]);

// Setup function.
extern "C" [[noreturn]] void _setup(int argument_count, char* arguments[], char* environment[]);
extern "C" [[noreturn]] void _setup(int argument_count, char* arguments[], char* environment[]) {

    // Preinit functions defined by the linker script.
    extern function_pointer_type __preinit_array_start;
    extern function_pointer_type __preinit_array_end;
    for (function_pointer_type* function = &__preinit_array_start; function < &__preinit_array_end; ++function) {
        (*function)();
    }

    // Init functions defined by the linker script.
    extern function_pointer_type __init_array_start;
    extern function_pointer_type __init_array_end;
    for (function_pointer_type* function = &__init_array_start; function < &__init_array_end; ++function) {
        (*function)();
    }

    // Run main.
    int return_code = main(argument_count, arguments, environment);

    // Call atexit functions.
    while (atexit_function_count--) {
        atexit_functions[atexit_function_count]();
    }

    // Destructors defined by the linker script.
    extern function_pointer_type __fini_array_start;
    extern function_pointer_type __fini_array_end;
    for (function_pointer_type* function = &__fini_array_start; function < &__fini_array_end; ++function) {
        (*function)();
    }

    // Exit syscall.
    asm volatile("syscall" : : "a" (60), "D" (return_code));

    // Never return.
    for(;;);
}

////////////////////////////////////////////////////////////////////////////////
// Syscall for writing output to a file handle.
////////////////////////////////////////////////////////////////////////////////

static inline int sys_write(int file_handle, const void* buffer, __SIZE_TYPE__ size) {
    int characters_written;
    asm volatile("syscall\n"
        : "=a" (characters_written)
        : "a" (1), "D" (file_handle), "S" (buffer), "d" (size)
        : "rcx", "r11", "memory"
    );
    return characters_written;
}

////////////////////////////////////////////////////////////////////////////////
// Main function.
////////////////////////////////////////////////////////////////////////////////

int main(int argument_count, char* arguments[], char* environment[]) {
    static_cast<void>(argument_count);
    static_cast<void>(arguments);
    static_cast<void>(environment);

    constexpr static const char hello_world[] = "hello world!\n";
    sys_write(1, hello_world, sizeof(hello_world));

    return 0;
}
