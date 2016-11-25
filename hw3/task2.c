#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

void quick_sort(int *, int, int);
int choose_pivot(int *, int, int);
void swap_int(int *, int *);

/*************************************************************
 * FUNCTION NAME: quick_sort                                         
 * PARAMETER: 1)target_arr: array to be sorted 2)start_idx: starting index of an array 3)end_idx: ending index of an array                                              
 * PURPOSE: sorting the array in ascencding order using quick-sort 
 ************************************************************/
void quick_sort(int *target_arr, int start_idx, int end_idx)
{
    if(start_idx == end_idx)
        return;

    int pivot = choose_pivot(target_arr, start_idx, end_idx);

    if(start_idx != pivot)
        quick_sort(target_arr, start_idx, pivot - 1);
    if(end_idx != pivot)
        quick_sort(target_arr, pivot + 1, end_idx);
}

/*************************************************************
 * FUNCTION NAME: choose_pivot                                         
 * PARAMETER: 1)target_arr: array to be sorted 2)start_idx: starting index of an array 3)end_idx: ending index of an array                                              
 * PURPOSE: choose the pivot randomly, and sort the array to be make following conditions
 *          An element which is smaller than the pivot should be located left of the pivot
 *          An element which is bigger than the pivot should be located right of the pivot
 ************************************************************/
int choose_pivot(int *target_arr, int start_idx, int end_idx)
{
    int i;
    int pivot, pivot_value;
    int swap_idx = 1;
    bool is_bigger = false;

    pivot = rand() % (end_idx - start_idx + 1);
    pivot_value = target_arr[pivot + start_idx];
    
//    for (i = 0; i < end_idx - start_idx + 1; i++) {
//        printf("%d ", target_arr[start_idx + i]);
//    }
//    printf("pivot: %d\n", pivot_value);
    swap_int(&target_arr[start_idx], &target_arr[pivot + start_idx]);
        
    for (i = 1; i < end_idx - start_idx + 1; i++) {
        if(!is_bigger && target_arr[i + start_idx] >= pivot_value)
        {
            is_bigger = true;
        }
        else if(target_arr[i + start_idx] < pivot_value)
        {
            if(is_bigger)
            {
                swap_int(&target_arr[start_idx + i], &target_arr[start_idx + swap_idx]); 
                swap_idx++;
            }
            else
            {
                swap_idx++;
            }
        }
    }
    
    swap_int(&target_arr[start_idx], &target_arr[start_idx + swap_idx - 1]);

//    for (i = 0; i < end_idx - start_idx + 1; i++) {
//        printf("%d ", target_arr[start_idx + i]);
//    }
//    printf("\n");

    return start_idx + swap_idx - 1; 
}

/*************************************************************
 * FUNCTION NAME: swap_int                                         
 * PARAMETER: 1)a: an integer to be swapped by b 1)b: an integer to be swapped by a                                              
 * PURPOSE: swap two integers 
 ************************************************************/
void swap_int(int *a, int *b)
{
    int temp;

    temp = *b;
    *b = *a;
    *a = temp;
}

int main(int argc, const char *argv[])
{
    int *input_arr = (int *)malloc((argc - 1) * sizeof(int));
    int i;

    if(argc == 1)
    {
        printf("input_arr is empty: ex) ./task2 3 5 1 2 3 5 10\n");
        return 0;
    }

    for (i = 0; i < argc - 1; i++) {
        input_arr[i] = atoi(argv[i + 1]);
    }
    srand(time(NULL));
    quick_sort(input_arr, 0, argc - 2);

    printf("sorted: ");
    for (i = 0; i < argc - 1; i++) {
        printf("%d ", input_arr[i]);
    }
    printf("\n");
    return 0;
}
