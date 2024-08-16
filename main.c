//docker run --rm -it -v "${PWD}:/home/devuser/work" qsrahmans/i686-elf-gcc:dev bash -c "i686-elf-gcc -Wa,--gdwarf2 -Wa,-march=pentium -c -o portASM.o lib/freertos_kernel/portable/GCC/IA32_flat/portASM.S"
#include <stdio.h>
#include <stdlib.h>

#include <xbox.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>
#include <semphr.h>

static void exampleTask( void * parameters )
{
    /* Unused parameters. */
    ( void ) parameters;

    for( ; ; )
    {
        /* Example Task Code */
        vTaskDelay( 1000 ); /* delay 100 ticks */
        printf("Thread1\r\n");
    }
}

static void exampleTask1( void * parameters )
{
    /* Unused parameters. */
    ( void ) parameters;

    for( ; ; )
    {
        /* Example Task Code */
        vTaskDelay( 1000 ); /* delay 100 ticks */
        printf("Thread2\r\n");
    }
}

int main(void) {
    int used = 1;
    char memory[64];

    float a= 9.1f;
    float b = 3.2f;
    float c = a * b;

    printf( "Example FreeRTOS Project\n" );

    snprintf(memory, sizeof(memory), "Hello, World!! %d %f\r\n", used, c);
    printf(memory);

    static StaticTask_t exampleTaskTCB;
    static StackType_t exampleTaskStack[ configMINIMAL_STACK_SIZE ];
    static StaticTask_t exampleTaskTCB1;
    static StackType_t exampleTaskStack1[ configMINIMAL_STACK_SIZE ];

    ( void ) xTaskCreateStatic( exampleTask,
                                "example",
                                configMINIMAL_STACK_SIZE,
                                NULL,
                                configMAX_PRIORITIES - 1U,
                                &( exampleTaskStack[ 0 ] ),
                                &( exampleTaskTCB ) );

    ( void ) xTaskCreateStatic( exampleTask1,
                                "example",
                                configMINIMAL_STACK_SIZE,
                                NULL,
                                configMAX_PRIORITIES - 1U,
                                &( exampleTaskStack1[ 0 ] ),
                                &( exampleTaskTCB1 ) );

    /* Start the scheduler. */
    vTaskStartScheduler();

    for( ; ; )
    {
        /* Should not reach here. */
    }

    return 0;
}

void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                        char * pcTaskName )
    {
        /* Check pcTaskName for the name of the offending task,
         * or pxCurrentTCB if pcTaskName has itself been corrupted. */
        ( void ) xTask;
        ( void ) pcTaskName;
    }