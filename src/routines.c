#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include <routines.h>
#include <queue.h>
#include <random.h>

// PARAMETERS
int n_students;
int n_groups;
int n_tutors;
int time_limit;

// SYNCHRONIZATION
pthread_cond_t student_arrived;
pthread_cond_t student_receive_id;
pthread_cond_t teacher_receive_ack;
pthread_cond_t all_students_assigned;
pthread_cond_t lab_ready;
pthread_cond_t lab_complete;
pthread_cond_t update_for_labs;
pthread_cond_t join_lab_broadcast;
pthread_cond_t student_joined_lab;
pthread_cond_t leave_lab_broadcast;
pthread_cond_t student_left_lab;
pthread_cond_t tutor_leave;

pthread_mutex_t student_arrival_lock;
pthread_mutex_t group_allocate;
pthread_mutex_t lab_transition;

// STATES
struct student* students;
struct group* groups;
struct lab* tutor_labs;

struct queue* available_labs;
struct queue* labs_to_free;

// local states
int students_present = 0;
int students_assigned = 0;
int cur_lab_started = 0;
bool no_students_left = false;

void* teacher_routine(void* arg)
{
    printf("Teacher: I’m waiting for all students to arrive.\n");

    // wait for all students to arrive
    pthread_mutex_lock(&student_arrival_lock);

    while (students_present < n_students)
    {
        pthread_cond_wait(&student_arrived, &student_arrival_lock);
    }

    pthread_mutex_unlock(&student_arrival_lock);
    // students have all arrived

    // assigning group ids
    printf(
        "Teacher: All students have arrived. "
        "I start to assign group ids to students.\n"
    );

    for (int i = 0; i < n_students; i++)
    {
        int skips = randint(0, n_students - i);
        int sid = -1;

        for (int j = 0; j < n_students; j++)
        {
            if (students[j].group_id == -1) // not allocated
            {
                if (skips <= 0)
                {
                    sid = j;
                    break;
                }

                skips--;
            }
        }

        // allocate student to group
        pthread_mutex_lock(&group_allocate);

        students[sid].group_id = i % n_groups;
        printf(
            "Teacher: student %d is in group %d.\n",
            sid,
            students[sid].group_id
        );
        students_assigned++;

        pthread_cond_broadcast(&student_receive_id);

        while (!students[sid].acked_teacher)
        {
            pthread_cond_wait(&teacher_receive_ack, &group_allocate);
        }

        pthread_mutex_unlock(&group_allocate);
        // allocated and received ack
    }
    // all students assigned to group

    // start labs
    printf("Teacher: I’m waiting for lab rooms to become available.\n");
    pthread_cond_broadcast(&all_students_assigned);

    // lab loop
    pthread_mutex_lock(&lab_transition);
    for (int i = 0; i < n_groups; i++)
    {
        // wait for a free lab
        while (queue_len(available_labs) == 0)
        {
            pthread_cond_wait(&lab_ready, &lab_transition);
        }

        // queue should be enq before signal, so should never be empty
        int chosen_lab = queue_deq(available_labs);

        printf(
            "Teacher: The lab %d is now available. Students in group %d can "
            "enter the room and start your lab exercise.\n",
            chosen_lab,
            i
        );

        tutor_labs[chosen_lab].cur_group = i;
        pthread_cond_broadcast(&update_for_labs);
    }

    no_students_left = true;

    pthread_mutex_unlock(&lab_transition);
    // labs finished

    // free tutors
    pthread_mutex_lock(&lab_transition);

    int labs_freed = 0;

    for (int i = 0; i < n_tutors; i++)
    {
        if (tutor_labs[i].cur_group == GROUP_NOT_ASSIGNED)
        {
            queue_enq(labs_to_free, i);
        }
    }

    while (labs_freed < n_tutors)
    {
        while (queue_len(labs_to_free) == 0)
        {
            pthread_cond_wait(&lab_complete, &lab_transition);
        }

        int lab_to_free = queue_deq(labs_to_free);

        printf(
            "Teacher: There are no students waiting. "
            "Tutor %d, you can go home now.\n",
            lab_to_free
        );
        tutor_labs[lab_to_free].can_leave = true;
        pthread_cond_broadcast(&update_for_labs);

        while (!tutor_labs[lab_to_free].acked_teacher)
        {
            // reusing teacher_receive_ack conditional as first stage of its
            // use with students assigned to groups does not overlap with
            // this second stage of labs finishing

            // - all students are assigned to groups and then tutors are excused
            pthread_cond_wait(&teacher_receive_ack, &lab_transition);
        }

        labs_freed++;
    }
    pthread_mutex_unlock(&lab_transition);

    printf("Teacher: All students and tutors are left. I can now go home\n");

    return NULL;
}

void* lab_routine(void* arg)
{
    int id = *((int*) arg);

    // wait for group allocation completion
    pthread_mutex_lock(&group_allocate);
    while (students_assigned < n_students)
    {
        pthread_cond_wait(&all_students_assigned, &group_allocate);
    }
    pthread_mutex_unlock(&group_allocate);

    // start labs in order
    pthread_mutex_lock(&lab_transition);
    while (cur_lab_started < id)
    {
        pthread_cond_wait(&lab_ready, &lab_transition);
    }
    pthread_mutex_unlock(&lab_transition);
    // right order now

    // group allocation finished and tutors can start running labs
    while (!tutor_labs[id].can_leave)
    {
        pthread_mutex_lock(&lab_transition);

        printf(
            "Tutor %d: The lab room %d is vacated and ready for one group\n",
            id,
            id
        );

        cur_lab_started++;
        queue_enq(available_labs, id);
        pthread_cond_broadcast(&lab_ready);

        while (
            !tutor_labs[id].can_leave &&
            tutor_labs[id].cur_group == LAB_EMPTY // lab not chosen yet
        ) {
            pthread_cond_wait(&update_for_labs, &lab_transition);
        }
        pthread_mutex_unlock(&lab_transition);

        if (tutor_labs[id].can_leave)
        {
            break;
        }

        struct group* group_chosen = &groups[tutor_labs[id].cur_group];
        group_chosen->assigned_lab = id;
        group_chosen->num_in_lab = 0;

        pthread_cond_broadcast(&join_lab_broadcast);

        // wait for students
        pthread_mutex_lock(&lab_transition);
        while (group_chosen->num_in_lab < group_chosen->num_members)
        {
            pthread_cond_wait(&student_joined_lab, &lab_transition);
        }
        pthread_mutex_unlock(&lab_transition);

        // all students in - do lab
        printf(
            "Tutor %d: All students in group %d have entered the room %d. "
            "You can start your exercise now.\n",
            id,
            tutor_labs[id].cur_group,
            id
        );

        // * 2 in random function and then / 2.0 to maintain integral property
        double time_to_take = randint(time_limit, time_limit * 2) / 2.0;
        usleep((unsigned int) (time_to_take * 1e6));

        // completed lab
        printf(
            "Tutor %d: Students in group %d have completed the lab "
            "exercise in %.1g units of time. You may leave this room now.\n",
            id,
            tutor_labs[id].cur_group,
            time_to_take
        );

        pthread_mutex_lock(&lab_transition);

        tutor_labs[id].cur_group = GROUP_NOT_ASSIGNED;
        group_chosen->assigned_lab = NO_LAB_ASSIGNED;

        pthread_cond_broadcast(&leave_lab_broadcast);

        // group is only assigned to lab once so can safely and dependably
        // calculate students left in lab from group data struct
        while (group_chosen->num_in_lab > 0)
        {
            pthread_cond_wait(&student_left_lab, &lab_transition);
        }

        if (no_students_left)
        {
            queue_enq(labs_to_free, id);
        }
        else
        {
            queue_enq(available_labs, id);
        }

        pthread_cond_broadcast(&lab_complete);

        pthread_mutex_unlock(&lab_transition);
    }

    // wait to be excused
    pthread_mutex_lock(&lab_transition);

    while (!tutor_labs[id].can_leave)
    {
        pthread_cond_wait(&tutor_leave, &lab_transition);
    }

    printf("Tutor %d: Thanks Teacher. Bye!\n", id);
    tutor_labs[id].acked_teacher = true;
    pthread_cond_broadcast(&teacher_receive_ack);

    pthread_mutex_unlock(&lab_transition);
    // excused

    return NULL;
}

void* student_routine(void* arg)
{
    int id = *((int*) arg);

    printf(
        "Student %d: I have arrived and wait for being assigned to a group.\n",
        id
    );

    pthread_mutex_lock(&student_arrival_lock);
    students_present++;
    pthread_cond_broadcast(&student_arrived);
    pthread_mutex_unlock(&student_arrival_lock);

    // wait to be allocated
    pthread_mutex_lock(&group_allocate);

    while (students[id].group_id == STUDENT_NOT_ASSIGNED)
    {
        pthread_cond_wait(&student_receive_id, &group_allocate);
    }

    // allocated
    int allocated_group = students[id].group_id;
    printf(
        "Student %d: OK, I’m in group %d and "
        "waiting for my turn to enter a lab room.\n",
        id,
        allocated_group
    );
    students[id].acked_teacher = true;

    pthread_cond_signal(&teacher_receive_ack);

    pthread_mutex_unlock(&group_allocate);
    // group allocation wait-ack process complete

    // wait to be allocated to a lab
    pthread_mutex_lock(&lab_transition);

    while (groups[allocated_group].assigned_lab == NO_LAB_ASSIGNED)
    {
        pthread_cond_wait(&join_lab_broadcast, &lab_transition);
    }

    int assigned_lab = groups[allocated_group].assigned_lab;
    groups[allocated_group].num_in_lab++;

    printf(
        "Student %d in group %d: My group is called. "
        "I will enter the lab room %d now.\n",
        id,
        allocated_group,
        groups[allocated_group].assigned_lab
    );

    pthread_cond_broadcast(&student_joined_lab);

    pthread_mutex_unlock(&lab_transition);

    // wait for lab to finish
    pthread_mutex_lock(&lab_transition);

    // lab not over
    while (groups[allocated_group].assigned_lab == assigned_lab)
    {
        pthread_cond_wait(&leave_lab_broadcast, &lab_transition);
    }

    printf(
        "Student %d in group %d: Thanks Tutor %d. Bye!\n",
        id,
        allocated_group,
        assigned_lab
    );

    groups[allocated_group].num_in_lab--;
    pthread_cond_broadcast(&student_left_lab);

    pthread_mutex_unlock(&lab_transition);

    return NULL;
}
