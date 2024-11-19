#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

void* sleep_and_print( void* arg )
{
    int num = *(int*)arg;
    sleep(num);
    printf( "%d ", num );
    free(arg);
    return NULL;
}

void sleep_sort( int* arr, int size )
{
    pthread_t* threads = malloc( size * sizeof(pthread_t) );

    for( int i = 0; i < size; ++i )
	{
        int* num = malloc( sizeof(int) );
        *num = arr[i];
        pthread_create( &threads[i], NULL, sleep_and_print, num );
    }

    for (int i = 0; i < size; i++)
        pthread_join( threads[i], NULL );

    free(threads);
}

int main() {
    int arr[] = { 4, 3, 1, 2, 5 };
    int size = ( sizeof(arr) / sizeof(arr[0]) );

    for( int i = 0; i < size; ++i )
        printf( "%d ", arr[i] );
	printf("\n");
    sleep_sort( arr, size );
    return 0;
}
