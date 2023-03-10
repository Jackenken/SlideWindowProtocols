#include "sender.h"   // sender.h,

void init_sender(Sender * sender, int id)
{
    //TODO: You should fill in this function as necessary
//    fprintf(stderr,"start init_sender\n");
    sender->send_id = id;
    sender->input_cmdlist_head = NULL;
    sender->input_framelist_head = NULL;
    sender->keeplist_head = NULL;
    sender->swp = (SWP*) malloc(sizeof(SWP)*glb_receivers_array_length);
    for (int i = 0; i < glb_receivers_array_length; i++)
    {
        SWP* swp = sender->swp+i;
        swp->noackSeq=0;
        swp->noackOffset=0;
        swp->sendSeq = 0;
        swp->lastSeq = -1;
        for(int j=0; j < WINDOWS_SIZE; j++){
            swp->send_times[j].tv_sec=0;
            swp->send_times[j].tv_usec=0;
            swp->ack[j]=0;
        }
    }
//    fprintf(stderr,"end init_sender\n");
}

struct timeval * sender_get_next_expiring_timeval(Sender * sender)
{
    //TODO: You should fill in this function so that it returns the next timeout that should occur
    struct timeval * expiring_timeval = (struct timeval*)malloc(sizeof(struct timeval));
    int input_cmd_length = ll_get_length(sender->input_cmdlist_head);
    int inframe_queue_length = ll_get_length(sender->input_framelist_head);
    int input_keeplist_length = ll_get_length(sender->keeplist_head);
    if (input_cmd_length == 0 &&inframe_queue_length == 0 && input_keeplist_length==0){
        expiring_timeval->tv_sec = 0;
        expiring_timeval->tv_usec = 100000;
        return expiring_timeval;
    }
    return NULL;
}


void handle_incoming_acks(Sender * sender,
                          LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling incoming ACKs
    //    1) Dequeue the ACK from the sender->input_framelist_head
    //    2) Convert the char * buffer to a Frame data type
    //    3) Check whether the frame is corrupted
    //    4) Check whether the frame is for this sender
    //    5) Do sliding window protocol for sender/receiver pair

//    fprintf(stderr,"start handle_incoming_acks\n");


    int input_framelist_length = ll_get_length(sender->input_framelist_head);

    input_framelist_length = ll_get_length(sender->input_framelist_head);

    while (input_framelist_length > 0) {
        LLnode* ll_input_frame_node = ll_pop_node(&sender->input_framelist_head);
        input_framelist_length = ll_get_length(sender->input_framelist_head);
        Frame* frame = (Frame*)ll_input_frame_node->value;
        free(ll_input_frame_node);
        if(frame->recv_id==sender->send_id){
            uint16_t fcs = crc16((char*)frame,MAX_FRAME_SIZE);
            if(fcs==0){
                fprintf(stderr,"%d ack %d, seq =  %d\n",frame->send_id,frame->recv_id,frame->seq);
                int seq = frame->seq;
                SWP* swp = sender->swp+frame->send_id;
                int flag = 0;
                if((swp->noackSeq+WINDOWS_SIZE)>MAX_SEQ_SIZE){
                    if((seq>=swp->noackSeq&&seq<=MAX_SEQ_SIZE-1) ||
                       (0<=seq&&seq<=(swp->noackSeq+WINDOWS_SIZE-1)%MAX_SEQ_SIZE))
                        flag = 1;
                }else{
                    if(seq>=swp->noackSeq&&seq<=(swp->noackSeq+WINDOWS_SIZE-1))
                        flag = 1;
                }
                if(flag == 1){
                    int index = (seq-swp->noackSeq+swp->noackOffset)%WINDOWS_SIZE;
                    swp->ack[index] = 1;
                    swp->send_times[index].tv_sec=0;
                    swp->send_times[index].tv_usec=0;
                }
                while(swp->ack[(swp->noackSeq)%WINDOWS_SIZE]==1){
                    swp->ack[(swp->noackSeq)%WINDOWS_SIZE]=0;
                    swp->noackSeq = (swp->noackSeq+1)%MAX_SEQ_SIZE;
                    swp->noackOffset = swp->noackSeq%WINDOWS_SIZE;
                }
            }
        }
//        free(frame);
    }

//    fprintf(stderr,"end handle_incoming_acks\n");

}


void handle_input_cmds(Sender * sender,
                       LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling input cmd
    //    1) Dequeue the Cmd from sender->input_cmdlist_head
    //    2) Convert to Frame
    //    3) Set up the frame according to the sliding window protocol
    //    4) Compute CRC and add CRC to Frame

//    fprintf(stderr,"start handle_input_cmds\n");


    int input_cmd_length = ll_get_length(sender->input_cmdlist_head);


    //Recheck the command queue length to see if stdin_thread dumped a command on us
    input_cmd_length = ll_get_length(sender->input_cmdlist_head);
    while (input_cmd_length > 0)
    {
        //Pop a node off and update the input_cmd_length
        LLnode * ll_input_cmd_node = ll_pop_node(&sender->input_cmdlist_head);
        input_cmd_length = ll_get_length(sender->input_cmdlist_head);

        //Cast to Cmd type and free up the memory for the node
        Cmd * outgoing_cmd = (Cmd *) ll_input_cmd_node->value;
        free(ll_input_cmd_node);


        //DUMMY CODE: Add the raw char buf to the outgoing_frames list
        //NOTE: You should not blindly send this message out!
        //      Ask yourself: Is this message actually going to the right receiver (recall that default behavior of send is to broadcast to all receivers)?
        //                    Does the receiver have enough space in in it's input queue to handle this message?
        //                    Were the previous messages sent to this receiver ACTUALLY delivered to the receiver?
        int msg_length = (int)strlen(outgoing_cmd->message);
        if (msg_length > FRAME_PAYLOAD_SIZE+1)
        {
            //Do something about messages that exceed the frame size
//            printf("<SEND_%d>: sending messages of length greater than %d is not implemented\n", sender->send_id, MAX_FRAME_SIZE);
            int count;
            if(msg_length%FRAME_PAYLOAD_SIZE==0){//????????????
                count = msg_length/FRAME_PAYLOAD_SIZE;
            }else{
                count = msg_length/FRAME_PAYLOAD_SIZE+1;
            }
            for(int i=1;i<=count;i++){
                Frame * keeping_frame = (Frame *) malloc (sizeof(Frame));
                keeping_frame->send_id=outgoing_cmd->src_id;
                keeping_frame->recv_id=outgoing_cmd->dst_id;
                if(i==count){
                    memcpy(keeping_frame->data,outgoing_cmd->message+(i-1)*FRAME_PAYLOAD_SIZE,msg_length%FRAME_PAYLOAD_SIZE);
                    keeping_frame->type = 3;
                }else{
                    if(i==1){
                        keeping_frame->type = 1;
                    }else{
                        keeping_frame->type = 2;
                    }
                    memcpy(keeping_frame->data,outgoing_cmd->message+(i-1)*FRAME_PAYLOAD_SIZE,FRAME_PAYLOAD_SIZE);
                    keeping_frame->data[FRAME_PAYLOAD_SIZE]='\0';
                }
                char * outgoing_charbuf = convert_frame_to_char(keeping_frame);
                ll_append_node(&sender->keeplist_head,
                               outgoing_charbuf);
            }
        }
        else
        {
            //This is probably ONLY one step you want
            Frame * outgoing_frame = (Frame *) malloc (sizeof(Frame));
            strcpy(outgoing_frame->data, outgoing_cmd->message);
            outgoing_frame->send_id=outgoing_cmd->src_id;
            outgoing_frame->recv_id=outgoing_cmd->dst_id;
            outgoing_frame->type=0;

            //At this point, we don't need the outgoing_cmd
            free(outgoing_cmd->message);
            free(outgoing_cmd);

            //Convert the message to the outgoing_charbuf
            char * outgoing_charbuf = convert_frame_to_char(outgoing_frame);
            ll_append_node(&sender->keeplist_head,
                           outgoing_charbuf);
//            free(outgoing_frame);
        }
    }

    int keep_len =  ll_get_length(sender->keeplist_head);
//    int flag=1;
//    fprintf(stderr,"keeplist_head\n");
//    ll_print_values(sender->keeplist_head);

    LLnode *h=sender->keeplist_head;
    int count = 0;
    while (keep_len-->0) {
//        flag=0;
//        fprintf(stderr,"keeplist_head: keep_len=%d\n",keep_len);
//        ll_print_values(sender->keeplist_head);
        Frame *frame = convert_char_to_frame(h->value);
        SWP *swp = sender->swp + frame->recv_id;
        LLnode *now=h;
        h=h->next;
//        if((swp->noackSeq + WINDOWS_SIZE - 1)<MAX_SEQ_SIZE) {
        if (  (swp->noackSeq!=((swp->lastSeq+1)%MAX_SEQ_SIZE))&&
              (( WINDOWS_SIZE - 1)  <= ((swp->lastSeq-swp->noackSeq+MAX_SEQ_SIZE)%MAX_SEQ_SIZE))) {
            fprintf(stderr, "noackSeq = %d, lastSeq = %d \n", swp->noackSeq, swp->lastSeq);
            fprintf(stderr, "%d: full\n", frame->recv_id);
            if((++count)>=10)
                break;
            continue;
        }
        count = 0;
        ll_pop_nth_node(&sender->keeplist_head,now);


        frame->seq = swp->sendSeq;
        swp->sendSeq = (swp->sendSeq + 1) % MAX_SEQ_SIZE;
        swp->lastSeq = frame->seq;
        uint16_t fcs = crc16(convert_frame_to_char(frame), MAX_FRAME_SIZE - 2);
        frame->FCS[0] = fcs >> 8;
        frame->FCS[1] = fcs & 0xFF;

//        //Convert the message to the outgoing_charbuf
        char *outgoing_charbuf = convert_frame_to_char(frame);
        ll_append_node(outgoing_frames_head_ptr,
                       outgoing_charbuf);
        fprintf(stderr,"%d send to %d, data =  %s\n",frame->send_id,frame->recv_id,frame->data);

        int seq = frame->seq;
        int index = (seq - swp->noackSeq + swp->noackOffset) % WINDOWS_SIZE;
        struct timeval curr_timeval;
        gettimeofday(&curr_timeval, NULL);
        swp->send_times[index] = curr_timeval;
        swp->cache[index] = *frame;
//        free(frame);
    }

//    fprintf(stderr,"end handle_input_cmds\n");


}


void handle_timedout_frames(Sender * sender,
                            LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling timed out datagrams
    //    1) Iterate through the sliding window protocol information you maintain for each receiver
    //    2) Locate frames that are timed out and add them to the outgoing frames
    //    3) Update the next timeout field on the outgoing frames

//    fprintf(stderr,"start handle_timedout_frames\n");

    for (int j = 0; j < glb_receivers_array_length; j++){
        SWP* swp = sender->swp+j;
        for (int i = 0; i < WINDOWS_SIZE; i++){
            struct timeval    curr_timeval;
            struct timeval    old_timeval;
            if(swp->ack[i]==0&&swp->send_times[i].tv_sec!=0&&swp->send_times[i].tv_usec!=0){
                gettimeofday(&curr_timeval, NULL);
                old_timeval = swp->send_times[i];
                if(timeval_usecdiff(&old_timeval,&curr_timeval)>100000){
                    Frame resendframe = swp->cache[i];
                    char * outgoing_charbuf = convert_frame_to_char(&resendframe);
                    ll_append_node(outgoing_frames_head_ptr,
                                   outgoing_charbuf);
                    struct timeval nowtime;
                    gettimeofday(&nowtime, NULL);
                    swp->send_times[i] = nowtime;
                }
            }
        }

    }

//    fprintf(stderr,"end handle_timedout_frames\n");

}


void * run_sender(void * input_sender)
{    
    struct timespec   time_spec;
    struct timeval    curr_timeval;
    const int WAIT_SEC_TIME = 0;
    const long WAIT_USEC_TIME = 100000;
    Sender * sender = (Sender *) input_sender;    
    LLnode * outgoing_frames_head;
    struct timeval * expiring_timeval;
    long sleep_usec_time, sleep_sec_time;
    
    //This incomplete sender thread, at a high level, loops as follows:
    //1. Determine the next time the thread should wake up
    //2. Grab the mutex protecting the input_cmd/inframe queues
    //3. Dequeues messages from the input queue and adds them to the outgoing_frames list
    //4. Releases the lock
    //5. Sends out the messages

    pthread_cond_init(&sender->buffer_cv, NULL);
    pthread_mutex_init(&sender->buffer_mutex, NULL);

    while(1)
    {    
        outgoing_frames_head = NULL;

        //Get the current time
        gettimeofday(&curr_timeval, 
                     NULL);

        //time_spec is a data structure used to specify when the thread should wake up
        //The time is specified as an ABSOLUTE (meaning, conceptually, you specify 9/23/2010 @ 1pm, wakeup)
        time_spec.tv_sec  = curr_timeval.tv_sec;
        time_spec.tv_nsec = curr_timeval.tv_usec * 1000;

        //Check for the next event we should handle
        expiring_timeval = sender_get_next_expiring_timeval(sender);

        //Perform full on timeout
        if (expiring_timeval == NULL)
        {
            time_spec.tv_sec += WAIT_SEC_TIME;
            time_spec.tv_nsec += WAIT_USEC_TIME * 1000;
        }
        else
        {
            //Take the difference between the next event and the current time
            sleep_usec_time = timeval_usecdiff(&curr_timeval,
                                               expiring_timeval);

            //Sleep if the difference is positive
            if (sleep_usec_time > 0)
            {
                sleep_sec_time = sleep_usec_time/1000000;
                sleep_usec_time = sleep_usec_time % 1000000;   
                time_spec.tv_sec += sleep_sec_time;
                time_spec.tv_nsec += sleep_usec_time*1000;
            }   
        }

        //Check to make sure we didn't "overflow" the nanosecond field
        if (time_spec.tv_nsec >= 1000000000)
        {
            time_spec.tv_sec++;
            time_spec.tv_nsec -= 1000000000;
        }

        
        //*****************************************************************************************
        //NOTE: Anything that involves dequeing from the input frames or input commands should go 
        //      between the mutex lock and unlock, because other threads CAN/WILL access these structures
        //*****************************************************************************************
        pthread_mutex_lock(&sender->buffer_mutex);

        //Check whether anything has arrived
        int input_cmd_length = ll_get_length(sender->input_cmdlist_head);
        int inframe_queue_length = ll_get_length(sender->input_framelist_head);
        
        //Nothing (cmd nor incoming frame) has arrived, so do a timed wait on the sender's condition variable (releases lock)
        //A signal on the condition variable will wakeup the thread and reaquire the lock
        if (input_cmd_length == 0 &&
            inframe_queue_length == 0)
        {
            
            pthread_cond_timedwait(&sender->buffer_cv, 
                                   &sender->buffer_mutex,
                                   &time_spec);
        }
        //Implement this
        handle_incoming_acks(sender,
                             &outgoing_frames_head);

        //Implement this
        handle_input_cmds(sender,
                          &outgoing_frames_head);

        pthread_mutex_unlock(&sender->buffer_mutex);


        //Implement this
        handle_timedout_frames(sender,
                               &outgoing_frames_head);

        //CHANGE THIS AT YOUR OWN RISK!
        //Send out all the frames
        int ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        
        while(ll_outgoing_frame_length > 0)
        {
            LLnode * ll_outframe_node = ll_pop_node(&outgoing_frames_head);
            char * char_buf = (char *)  ll_outframe_node->value;

            //Don't worry about freeing the char_buf, the following function does that
            send_msg_to_receivers(char_buf);

            //Free up the ll_outframe_node
            free(ll_outframe_node);

            ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        }
    }
    pthread_exit(NULL);
    return 0;
}
