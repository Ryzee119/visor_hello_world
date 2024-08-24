// docker run --rm -it -v "${PWD}:/home/devuser/work" qsrahmans/i686-elf-gcc:dev bash -c "i686-elf-gcc -Wa,--gdwarf2
// -Wa,-march=pentium -c -o portASM.o lib/freertos_kernel/portable/GCC/IA32_flat/portASM.S"
#include <stdio.h>
#include <stdlib.h>

#include <xbox/xbox.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>
#include <semphr.h>

static void exampleTask(void *parameters)
{
    /* Unused parameters. */
    (void)parameters;

    TickType_t delay = pdMS_TO_TICKS(1000);
    for (;;) {
        /* Example Task Code */
        uint32_t a = xbox_timer_query_performance_counter();
        vTaskDelay(delay); /* delay 100 ticks */
        uint32_t b = xbox_timer_query_performance_counter();
        printf("Thread1 b - a %u\r\n", ((b - a) * 1000) / xbox_timer_query_performance_frequency());
    }
}

int main(void)
{
    int used = 1;
    char memory[64];

    float a = 9.1f;
    float b = 3.2f;
    float c = a * b;

    printf("Example FreeRTOS Project\n");

    snprintf(memory, sizeof(memory), "Hello, World!! %d %f\r\n", used, c);
    printf("%s\n", memory);

    static StaticTask_t exampleTaskTCB;
    static StackType_t exampleTaskStack[4096];
    (void)xTaskCreateStatic(exampleTask, "example", 4096, NULL, configMAX_PRIORITIES - 1U, &(exampleTaskStack[0]),
                            &(exampleTaskTCB));

    /* Start the scheduler. */
    vTaskStartScheduler();

    for (;;) {
        /* Should not reach here. */
    }

    return 0;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    /* Check pcTaskName for the name of the offending task,
     * or pxCurrentTCB if pcTaskName has itself been corrupted. */
    (void)xTask;
    (void)pcTaskName;
}