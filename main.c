#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include <queue.h>
#include <routines.h>

#define COND_INIT(IDENT) pthread_cond_init(&(IDENT), NULL)
#define MUTEX_INIT(IDENT) pthread_mutex_init(&(IDENT), NULL)

#define COND_DEST(IDENT) pthread_cond_destroy(&(IDENT))
#define MUTEX_DEST(IDENT) pthread_mutex_destroy(&(IDENT))


int main()
{
    srand(time(NULL));

    // synchronisation init
    COND_INIT(student_arrived);
    COND_INIT(student_receive_id);
    COND_INIT(teacher_receive_ack);
    COND_INIT(all_students_assigned);
    COND_INIT(lab_ready);
    COND_INIT(lab_complete);
    COND_INIT(update_for_labs);
    COND_INIT(join_lab_broadcast);
    COND_INIT(student_joined_lab);
    COND_INIT(leave_lab_broadcast);
    COND_INIT(student_left_lab);
    COND_INIT(tutor_leave);

    MUTEX_INIT(student_arrival_lock);
    MUTEX_INIT(group_allocate);
    MUTEX_INIT(lab_transition);

    // user input
    printf("Enter the following values for:\n");
    printf("N: total number of students in the class\n");
    scanf("%d", &n_students);

    printf("M: number of groups\n");
    scanf("%d", &n_groups);

    printf("K: number of tutors, or lab rooms\n");
    scanf("%d", &n_tutors);

    printf("T: time limit for each group of students to do the lab exercise\n");
    scanf("%d", &time_limit);

    // students
    students = (struct student*) malloc(n_students * sizeof(struct student));

    for (int i = 0; i < n_students; i++)
    {
        students[i].group_id = STUDENT_NOT_ASSIGNED;
        students[i].acked_teacher = false;
    }

    // groups
    groups = (struct group*) malloc(n_groups * sizeof(struct group));

    for (int i = 0; i < n_groups; i++)
    {
        groups[i].num_members = 0;
        groups[i].assigned_lab = NO_LAB_ASSIGNED;
        groups[i].num_in_lab = NO_LAB_ASSIGNED;
    }

    // tutors/labs
    tutor_labs = (struct lab*) malloc(n_tutors * sizeof(struct lab));

    for (int i = 0; i < n_tutors; i++)
    {
        tutor_labs[i].cur_group = LAB_EMPTY;
        tutor_labs[i].flagged_to_leave = false;
        tutor_labs[i].excused_by_teacher = false;
        tutor_labs[i].acked_teacher = false;
    }

    // queues
    available_labs = queue_create(n_tutors);
    labs_to_free = queue_create(n_tutors);

    // thread init -------------------------------------------------------------

    // teacher
    pthread_t teacher_thread;
    pthread_create(&teacher_thread, NULL, teacher_routine, NULL);

    // tutors/labs
    int* lab_ids = malloc(n_tutors * sizeof(int));
    pthread_t* lab_threads = malloc(n_tutors * sizeof(pthread_t));

    for (int i = 0; i < n_tutors; i++)
    {
        lab_ids[i] = i;
        pthread_create(
            &lab_threads[i],
            NULL,
            lab_routine,
            (void*) &lab_ids[i]
        );
    }

    // students
    int* sids = malloc(n_students * sizeof(int));
    pthread_t* student_threads = malloc(n_students * sizeof(pthread_t));

    for (int i = 0; i < n_students; i++)
    {
        sids[i] = i;
        pthread_create(
            &student_threads[i],
            NULL,
            student_routine,
            (void*) &sids[i]
        );
    }

    // join threads
    pthread_join(teacher_thread, NULL);

    for (int i = 0; i < n_tutors; i++)
    {
        pthread_join(lab_threads[i], NULL);
    }

    for (int i = 0; i < n_students; i++)
    {
        pthread_join(student_threads[i], NULL);
    }

    // end
    printf(
        "Main thread: All students have completed their lab exercises. "
        "This is the end of simulation.\n"
    );

    // destructors
    COND_DEST(student_arrived);
    COND_DEST(student_receive_id);
    COND_DEST(teacher_receive_ack);
    COND_DEST(all_students_assigned);
    COND_DEST(lab_ready);
    COND_DEST(lab_complete);
    COND_DEST(update_for_labs);
    COND_DEST(join_lab_broadcast);
    COND_DEST(student_joined_lab);
    COND_DEST(leave_lab_broadcast);
    COND_DEST(student_left_lab);
    COND_DEST(tutor_leave);

    MUTEX_DEST(student_arrival_lock);
    MUTEX_DEST(group_allocate);
    MUTEX_DEST(lab_transition);

    queue_destroy(&available_labs);
    queue_destroy(&labs_to_free);

    free(students);
    free(groups);
    free(tutor_labs);
    free(lab_ids);
    free(lab_threads);
    free(sids);
    free(student_threads);

    return 0;
}
