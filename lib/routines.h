#ifndef ROUTINES_H
#define ROUTINES_H 1

#include <stdbool.h>

#define GROUP_NOT_ASSIGNED (-1)
#define STUDENT_NOT_ASSIGNED (-1)
#define NO_LAB_ASSIGNED (-1)
#define LAB_EMPTY (-1)

// STRUCTS

struct student
{
    int group_id;
    bool acked_teacher;
};

struct group
{
    int num_members;
    int assigned_lab;
    int num_in_lab;
};

struct lab
{
    int cur_group;
    bool flagged_to_leave;
    bool excused_by_teacher;
    bool acked_teacher;
};

// PARAMETERS
extern int n_students;
extern int n_groups;
extern int n_tutors;
extern int time_limit;

// SYNCHRONIZATION
extern pthread_cond_t student_arrived;
extern pthread_cond_t student_receive_id;
extern pthread_cond_t teacher_receive_ack;
extern pthread_cond_t all_students_assigned;
extern pthread_cond_t lab_ready;
extern pthread_cond_t lab_complete;

/**
 * Either group is allocated to a lab, or all students have bene tutored
 * so signals empty labs at that time to leave
 */
extern pthread_cond_t update_for_labs;
extern pthread_cond_t join_lab_broadcast;
extern pthread_cond_t student_joined_lab;
extern pthread_cond_t leave_lab_broadcast;
extern pthread_cond_t student_left_lab;
extern pthread_cond_t tutor_leave;

extern pthread_mutex_t student_arrival_lock;
extern pthread_mutex_t group_allocate;
extern pthread_mutex_t lab_transition;

// STATES
extern struct student* students;
extern struct group* groups;
extern struct lab* tutor_labs;

extern struct queue* available_labs;
extern struct queue* labs_to_free;

void* teacher_routine(void* arg);
void* lab_routine(void* arg);
void* student_routine(void* arg);

#endif
