#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <memory>
#define STACK_SIZE 1024 * 64

ucontext_t ctx_main, ctx_func1;
int function1_finished = 0;

std::shared_ptr<int> cnt = std::make_shared<int>(0);

void function1() {
    printf("Function1 started\n");
    printf("Function1: Swapping back to main\n");
    swapcontext(&ctx_func1, &ctx_main);
    
    printf("Function1: Resumed again\n");
    printf("Function1: Finished\n");
    function1_finished = 1;
    auto it = cnt;
    printf("Function1: cnt use_count = %ld\n", it.use_count());
    // 函数执行到这里结束，正常返回
    // 由于 uc_link = &ctx_main，会自动跳回main
}

int main() {
    char stack1[STACK_SIZE];

    getcontext(&ctx_func1);
    ctx_func1.uc_stack.ss_sp = stack1;
    ctx_func1.uc_stack.ss_size = sizeof(stack1);
    ctx_func1.uc_link = &ctx_main;

    makecontext(&ctx_func1, function1, 0);

    printf("Main: Swapping to function1\n");
    swapcontext(&ctx_main, &ctx_func1);

    printf("Main: Resumed from function1\n");

    printf("Main: Swapping to function1 again\n");
    swapcontext(&ctx_main, &ctx_func1);
    
    // 如果function1正常返回，我们会执行到这里
    if (function1_finished) {
        printf("Main: function1 has finished execution and stack is freed\n");
    }
    printf("Main: cnt use_count = %ld\n", cnt.use_count());
    printf("Main: Exiting\n");
    return 0;
}