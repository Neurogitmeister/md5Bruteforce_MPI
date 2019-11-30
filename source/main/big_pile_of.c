#ifdef MEASURE_TIME
    {
        // Allreduce здесь одновременно выступает и синхронизацией вывода на экран

        double reduced_time, time_min, time_max, time_avg;
        // double time_compare;
        long long int time_trunc_a, time_trunc_b;
        const unsigned precision_max = 1000000000; // 9 знаков после запятой
        unsigned precision_curr;

    #ifdef TIME_CHECK
        if (commsize < N_CHECK) {
            MPI_Barrier(MPI_COMM_WORLD);
            sprintf(message,"Execution time on process %d: %lf\n", rank, time_sum);
            MPI_Print_in_rank_order(commsize, rank, message);
        }
    #endif

    unsigned curr_length = max_wanted_length + 1;

    MPI_Barrier(MPI_COMM_WORLD);
    if(!rank)
        printf("Execution time's mean on %d processes for word length (WL) :\n", commsize);

    #ifdef BENCHMARK
        // Проходим по массиву замеров для разных длин слова, а на последней итерации выводим результаты для суммы времен из time_sum.
        curr_length = min_wanted_length;

        for(; curr_length <= max_wanted_length + 1; curr_length++) {
            if(curr_length < max_wanted_length)
                reduced_time = times_single[curr_length - min_wanted_length];
            else
                reduced_time = time_sum;
    #else
        {
            reduced_time = time_sum;
    #endif
            
            MPI_Allreduce(&reduced_time, &time_min, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
            
            if(!rank) {
                if(curr_length == max_wanted_length + 1)
                    printf("WL %3d-%-3d", min_wanted_length, max_wanted_length);
                else {
                    printf("WL %3d%5s", curr_length," ");

                    if(curr_length < 7)
                        precision_curr = precision_max / (int)pow(10,curr_length);
                    else
                        precision_curr = 1000; // Переход на 3 знака до запятой
                }
            }

            // time_compare = reduced_time - reduced_time * 0.00001;
            // if(time_min > time_compare) {
            
            time_trunc_a = time_min * precision_curr;
            time_trunc_b = reduced_time * precision_curr;

            if(time_trunc_a == time_trunc_b) {
                printf("\tminimal : %lf seconds on rank %d;\n", time_min, rank);
            }

            MPI_Allreduce(&reduced_time, &time_max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

            // time_compare = reduced_time + reduced_time * 0.00001;
            // if(time_max < time_compare) {

            time_trunc_a = time_max * precision_curr;
            time_trunc_b = reduced_time * precision_curr;
            if(time_trunc_a == time_trunc_b) {
                printf("\t\tmaximal : %lf seconds on rank %d;\n", time_max, rank);
            }

            MPI_Allreduce(&reduced_time, &time_avg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

            if(!rank) {
                time_avg /= commsize;
                printf("\t\tmean :    %lf seconds.\n", time_avg);
            }
        
        } // Конец for

        if(!rank) 
            printf("Program counted total collisions after\n");

        MPI_Allreduce(&time_complete, &time_min, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);

        // time_compare = time_complete - time_complete * 0.00001;
        // if (time_min > time_compare) {

        time_trunc_a = time_min * precision_curr;
        time_trunc_b = time_complete * precision_curr;
        if(time_trunc_a == time_trunc_b) {
            printf("\tminimal : %lf seconds on rank %d;\n", time_min, rank);
        }

        MPI_Allreduce(&time_complete, &time_max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

        // time_compare = time_complete + time_complete * 0.00001;
        // if (time_max < time_compare) {

        time_trunc_a = time_max * 10000;
        time_trunc_b = time_complete * 10000;
        if(time_trunc_a == time_trunc_b) {
            printf("\tmaximal : %lf seconds on rank %d;\n", time_max, rank);
        }

        MPI_Allreduce(&time_complete, &time_avg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

        if(!rank) {
            time_avg /= commsize;
            printf("\tmean :    %lf seconds.\n", time_avg);
        }
    }
    #endif