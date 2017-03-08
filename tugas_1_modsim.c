#include "simlib.h"  
#include "string.h"            /* Required for use of simlib.c. */

#define EVENT_ARRIVAL           1  /* Event type for arrival of a job to the
                                    system. */
#define EVENT_DEPARTURE         2  /* Event type for departure of a job from a
                                    particular station. */
#define EVENT_END_SIMULATION    3  /* Event type for end of the simulation. */
#define STREAM_INTERARRIVAL     1  /* Random-number stream for interarrivals. */
#define STREAM_GROUP_TYPE       2  /* Random number stream for group types. */
#define STREAM_JOB_TYPE         3  /* Random-number stream for job types. */
#define STREAM_SERVICE_HOTFOOD  4  /* Random-number stream for service time in hot-food station . */
#define STREAM_SERVICE_SANDWICH 5  /* Random-number stream for service time in specialty-sandwich station . */
#define STREAM_SERVICE_DRINKS   6  /* Random-number stream for service time in drinks station . */
#define STREAM_CASHIER_HOTFOOD  7  /* Random-number stream for cashier time in hot-food station . */
#define STREAM_CASHIER_SANDWICH 8  /* Random-number stream for cashier time in specialty-sandwich station . */
#define STREAM_CASHIER_DRINKS   9  /* Random-number stream for cashier time in drinks station . */
#define MAX_NUM_STATIONS        5  /* Maximum number of stations. */
#define MAX_NUM_JOB_TYPES       3  /* Maximum number of job types. */
#define BOUNDS				    2
#define LOWER_BOUND			    1
#define UPPER_BOUND			    2
#define SAMPST_DELAYS        1  /* sampst variable for delays in queue(s). */


int   num_stations, num_job_types, num_arrival_count, num_job_arrive, i, j, num_machines[MAX_NUM_STATIONS + 1],
      num_tasks[MAX_NUM_JOB_TYPES +1],
      route[MAX_NUM_JOB_TYPES +1][MAX_NUM_STATIONS + 1],
      num_machines_busy[MAX_NUM_STATIONS + 1], job_type, task, shortest_length,shortest_queue;
float mean_interarrival, length_simulation, prob_distrib_job_type[26], prob_distrib_arrival_count[26],
      service_time[MAX_NUM_JOB_TYPES +1][ MAX_NUM_STATIONS + 1][BOUNDS + 1],
      accumulated_cashier_time[MAX_NUM_JOB_TYPES +1][ MAX_NUM_STATIONS + 1][BOUNDS + 1],ACT;
FILE  *infile, *outfile;


void arrive(int new_job){
    int station;
    if (new_job ==1){
        event_schedule(sim_time+expon(mean_interarrival,STREAM_INTERARRIVAL),EVENT_ARRIVAL);  
        job_type= random_integer(prob_distrib_job_type,STREAM_JOB_TYPE);       
        task = 1; 
        ACT = 0.0;
    }else if(new_job==2){
        job_type= random_integer(prob_distrib_job_type,STREAM_JOB_TYPE);       
        task = 1; 
        ACT = 0.0;
    }

    station = route[job_type][task];
    if(station ==4){
        int machine;
        for(machine = 1;machine<num_machines[station];++machine){
            if(list_size[8+machine]==0){
                sampst(0.0,station);
                //sampst(0.0,station+machine+job_type);
                list_file(FIRST,8+machine);
                transfer[3]=job_type;
                transfer[4]=task;
                transfer[5]=machine;
                ++num_machines_busy[station];
                event_schedule(sim_time+ACT,EVENT_DEPARTURE);
                return;
            }
        }

        shortest_length = list_size[8+1];
        shortest_queue =1;
        for(machine=2; machine<=num_machines[4];++machine){
            if(list_size[station+machine]<shortest_length){
                shortest_length = list_size[station+machine];
                shortest_queue = machine;
            }
        }

        transfer[1]=sim_time;
        transfer[2]=job_type;
        transfer[3]=task;
        transfer[4]=ACT;
        list_file(LAST,station+shortest_queue);
        list_file(LAST,station);
    }else{
        if(num_machines_busy[station] == num_machines[station]){
            transfer[1]=sim_time;
            transfer[2]=job_type;
            transfer[3]=task;
            transfer[4]=ACT;
            list_file(LAST,station);
        }else{
            int st_stream, act_stream;
            sampst(0.0, station);                              /* For station. */
            sampst(0.0, num_stations + job_type);              /* For job type. */
            ++num_machines_busy[station];
            timest((float)num_machines_busy[station],station);
            transfer[3]=job_type;
            transfer[4]=task;
            switch(station){
                case 1 : st_stream = STREAM_SERVICE_HOTFOOD; act_stream = STREAM_CASHIER_HOTFOOD; break;
                case 2 : st_stream = STREAM_SERVICE_SANDWICH; act_stream = STREAM_CASHIER_SANDWICH; break;
                case 3 : st_stream = STREAM_SERVICE_DRINKS; act_stream = STREAM_CASHIER_DRINKS; break;
            }
            ACT += uniform(accumulated_cashier_time[job_type][task][LOWER_BOUND],accumulated_cashier_time[job_type][task][UPPER_BOUND],act_stream);
            transfer[5]=ACT;
            event_schedule(sim_time + uniform(service_time[job_type][task][LOWER_BOUND],service_time[job_type][task][UPPER_BOUND],st_stream),EVENT_DEPARTURE);
        }

    }
}

void depart(){
    int station, job_type_queue, task_queue;
    job_type = transfer[3];
    task = transfer [4];
    station = route[job_type][task];

    if(station==4){
        int machine = transfer[5];
        if(list_size[station+machine]==0){
            list_remove(FIRST,8+machine);
        }else{
            list_remove(FIRST,station);
            list_remove(FIRST,station+machine);
            ACT = transfer[4]; 
            sampst(sim_time-transfer[1],station);
            transfer[3]=job_type;
            transfer[4]=task;
            transfer[5]=machine;
            event_schedule(sim_time+ACT,EVENT_DEPARTURE);
        }
    }else{
        if(list_size[station]==0){
            --num_machines_busy[station];
            timest((float)num_machines_busy[station],station); 
        }else{
            int st_stream,act_stream;
            list_remove(FIRST,station);
            sampst(sim_time - transfer[1],station);
            job_type_queue = transfer[2];
            task_queue = transfer[3];
            ACT = transfer[4];
            sampst(sim_time -transfer[1], num_stations+job_type_queue);
            transfer[3]=job_type_queue;
            transfer[4]=task_queue;
            
            switch(station){
                case 1 : st_stream = STREAM_SERVICE_HOTFOOD; act_stream = STREAM_CASHIER_HOTFOOD; break;
                case 2 : st_stream = STREAM_SERVICE_SANDWICH; act_stream = STREAM_CASHIER_SANDWICH; break;
                case 3 : st_stream = STREAM_SERVICE_DRINKS; act_stream = STREAM_CASHIER_DRINKS; break;
            }
            ACT += uniform(accumulated_cashier_time[job_type][task][LOWER_BOUND],accumulated_cashier_time[job_type][task][UPPER_BOUND],act_stream);            
            transfer[5]=ACT;
            event_schedule(sim_time + uniform(service_time[job_type][task][LOWER_BOUND],service_time[job_type][task][UPPER_BOUND],st_stream),EVENT_DEPARTURE);
        }
    }
    if(task < num_tasks[job_type]){
        ++task;
        arrive(3);
    }
}

void report(){
    int   i,machine;
    float overall_avg_job_tot_delay, avg_job_tot_delay, sum_probs, avg_num_in_queue;

    /* Compute the average total delay in queue for each job type and the
       overall average job total delay. */

    fprintf(outfile, "\n\n\n\nJob type     Average total delay in queue");
    overall_avg_job_tot_delay = 0.0;
    sum_probs                 = 0.0;
    avg_num_in_queue          = 0.0;
    for (i = 1; i <= num_job_types; ++i) {
        avg_job_tot_delay = sampst(0.0, -(num_stations + i)) * num_tasks[i];
        fprintf(outfile, "\n\n%4d%27.3f", i, avg_job_tot_delay);
        overall_avg_job_tot_delay += (prob_distrib_job_type[i] - sum_probs)
                                     * avg_job_tot_delay;
        sum_probs = prob_distrib_job_type[i];
    }
    fprintf(outfile, "\n\nOverall average job total delay =%10.3f\n",
            overall_avg_job_tot_delay);

    /* Compute the average number in queue, the average utilization, and the
       average delay in queue for each station. */

    fprintf(outfile,
           "\n\n\n Work      Average number      Average       Average delay       Maximum number");
    fprintf(outfile,
             "\nstation       in queue       utilization        in queue            in queue");
    for (j = 1; j < num_stations; ++j)
        fprintf(outfile, "\n\n%4d%17.3f%17.3f%17.3f%17.3f", j, filest(j),
                timest(0.0, -j) / num_machines[j], sampst(0.0, -j),timest (-1.0, -(TIM_VAR + j)));
    for (machine = 1; machine <= num_machines[4]; ++machine)
            fprintf(outfile, "\n\n%4d%17.3f%17.3f%17.3f%17.3f", machine, filest(machine),
                timest(0.0, -machine) , sampst(0.0, -machine),timest (-1.0, -(TIM_VAR + machine))); 
    
    fprintf(outfile, "\n\nDelays in station queue, in seconds:\n");
    out_sampst(outfile, 1, 3 );
    fprintf(outfile, "\n\nDelays in cashier queue, in seconds:\n");
    out_sampst(outfile,4+1,4+num_machines[4]);
    for (machine = 1; machine <= num_machines[4]; ++machine)
        avg_num_in_queue += filest(4+machine);
    fprintf(outfile, "\n\nWith%2d tellers, total average number in queue = %10.3f",
            num_machines[4],avg_num_in_queue);

    fprintf(outfile,
           "\n\n\nAverage number      Maximum number");
    fprintf(outfile,
             "\ncustomer              in queue");
    float avg_customer = 0 ;
    int max_customer = 0;
    for (j = 1; j < num_stations; ++j){
        avg_customer += filest(j);
        max_customer+=   timest (-1.0, -(TIM_VAR + j));
    }
    fprintf(outfile, "\n\n%3f%17d",avg_customer, max_customer);    
    

}

int main(){
	char finput[100] = "";
	char foutput[100] ="";
	printf("Config File:\n");
	scanf("%s",finput);
	infile = fopen(finput,"r");
	strcpy(foutput,finput);
	strcat(foutput,"_result.out");
	outfile = fopen(foutput,"w");

	fscanf(infile, "%d %d %d %f %f", &num_stations, &num_job_types, &num_arrival_count, &mean_interarrival, &length_simulation);
	for(i=1; i<=num_stations; ++i)
		fscanf(infile,"%d", &num_machines[i]);
	for(i=1; i<=num_job_types; ++i)
		fscanf(infile,"%d", &num_tasks[i]);
	for(i=1; i<=num_job_types; ++i){
		for(j=1; j<=num_tasks[i]; ++j)
			fscanf(infile,"%d", &route[i][j]);
		for(j=1; j<num_tasks[i]; ++j){
			fscanf(infile,"%f", &service_time[i][j][LOWER_BOUND]);
			fscanf(infile,"%f", &service_time[i][j][UPPER_BOUND]);
		}
		for(j=1; j<num_tasks[i]; ++j){
			fscanf(infile,"%f", &accumulated_cashier_time[i][j][LOWER_BOUND]);
			fscanf(infile,"%f", &accumulated_cashier_time[i][j][UPPER_BOUND]);
		}
	}
	for(i=1; i<=num_arrival_count; ++i){
        fscanf(infile,"%f", &prob_distrib_arrival_count[i]);
    }
	for(i=1; i<=num_job_types; ++i){
        fscanf(infile,"%f", &prob_distrib_job_type[i]);
    }

	fprintf(outfile, "Tugas 1 Modsim\n");
	fprintf(outfile, "Konfigurasi: %s\n\n\n", finput);
	fprintf(outfile, "Number of work stations%21d\n\n", num_stations);
    fprintf(outfile, "Number of machines in each station     ");
    for (j = 1; j <= num_stations; ++j)
        fprintf(outfile, "%5d", num_machines[j]);
    fprintf(outfile, "\n\nNumber of job types%25d\n\n", num_job_types);
    fprintf(outfile, "Number of tasks for each job type      ");
    for (i = 1; i <= num_job_types; ++i)
        fprintf(outfile, "%5d", num_tasks[i]);
    fprintf(outfile, "\n\nDistribution function of job types       ");
    for (i = 1; i <= num_job_types; ++i)
        fprintf(outfile, "%7.3f", prob_distrib_job_type[i]);
    fprintf(outfile, "\n\nDistribution function of arrival number  ");
    for (i = 1; i <= num_arrival_count; ++i)
        fprintf(outfile, "%7.3f", prob_distrib_arrival_count[i]);    
    fprintf(outfile, "\n\nMean interarrival time of jobs%18.2f seconds\n\n",
            mean_interarrival);
    fprintf(outfile, "Length of the simulation%25.1f in seconds\n\n\n",
            length_simulation);
    fprintf(outfile, "Job type     Work stations on route");
    for (i = 1; i <= num_job_types; ++i) {
        fprintf(outfile, "\n\n%4d        ", i);
        for (j = 1; j <= num_tasks[i]; ++j)
            fprintf(outfile, "%5d", route[i][j]);
    }
    fprintf(outfile, "\n\n\nJob type     ");
    fprintf(outfile, "Service time (in seconds) for successive tasks");
    for (i = 1; i <= num_job_types; ++i) {
        fprintf(outfile, "\n\n%4d    ", i);
        for (j = 1; j < num_tasks[i]; ++j){
            fprintf(outfile, "U(%0.2f,", service_time[i][j][LOWER_BOUND]);
            fprintf(outfile, "%0.2f)     ", service_time[i][j][UPPER_BOUND]);
        }
    }
    fprintf(outfile, "\n\n\nJob type     ");
    fprintf(outfile, "Accumulated cashier time (in seconds) for successive tasks");
    for (i = 1; i <= num_job_types; ++i) {
        fprintf(outfile, "\n\n%4d    ", i);
        for (j = 1; j < num_tasks[i]; ++j){
        	fprintf(outfile, "U(%0.2f,", accumulated_cashier_time[i][j][LOWER_BOUND]);
            fprintf(outfile, "%0.2f)     ", accumulated_cashier_time[i][j][UPPER_BOUND]);
        }
    }

    for(i=1;i<=num_stations; ++i)
        num_machines_busy[j] = 0;

    init_simlib();

    maxatr = 5;

    event_schedule(expon(mean_interarrival,STREAM_INTERARRIVAL),EVENT_ARRIVAL);    
    event_schedule(length_simulation,EVENT_END_SIMULATION);

    do{
        timing();
        switch(next_event_type){
            case EVENT_ARRIVAL: 
                num_job_arrive = random_integer(prob_distrib_arrival_count,STREAM_GROUP_TYPE);
                for(i=1;i<=num_job_arrive;++i){
                    if (i==1){
                        arrive(1);
                    }else{
                        arrive(2);
                    }
                }
                break;
            case EVENT_DEPARTURE: depart(); break;
            case EVENT_END_SIMULATION:report(); break;
        }
    }while(next_event_type!=EVENT_END_SIMULATION);
	fclose(infile);
	fclose(outfile);
}
