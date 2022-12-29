
#include "receiver.h"   // comment receiver.h,
void init_receiver(Receiver * receiver,
                   int id)
{
//    fprintf(stderr,"start init_receiver\n");

    receiver->recv_id = id;
    receiver->input_framelist_head = NULL;
    receiver->swp = (SWP*) malloc(sizeof(SWP)*glb_senders_array_length);
    for(int i=0;i<glb_senders_array_length;i++){
        SWP* swp = receiver->swp + i;
        swp->noackSeq=0;
        swp->noackOffset=0;
        for(int j=0; j < WINDOWS_SIZE; j++){
            swp->ack[j]=0;
        }
    }

//    fprintf(stderr,"end init_receiver\n");

}


void handle_incoming_msgs(Receiver * receiver,
                          LLnode ** outgoing_frames_head_ptr)
{
    //TODO: Suggested steps for handling incoming frames
    //    1) Dequeue the Frame from the sender->input_framelist_head
    //    2) Convert the char * buffer to a Frame data type
    //    3) Check whether the frame is corrupted
    //    4) Check whether the frame is for this receiver
    //    5) Do sliding window protocol for sender/receiver pair


//    fprintf(stderr,"start handle_incoming_msgs\n");


    int incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
    while (incoming_msgs_length > 0)
    {
        //Pop a node off the front of the link list and update the count
        LLnode * ll_inmsg_node = ll_pop_node(&receiver->input_framelist_head);
        incoming_msgs_length = ll_get_length(receiver->input_framelist_head);

        //DUMMY CODE: Print the raw_char_buf
        //NOTE: You should not blindly print messages!
        //      Ask yourself: Is this message really for me?
        //                    Is this message corrupted?
        //                    Is this an old, retransmitted message?           
        char * raw_char_buf = (char *) ll_inmsg_node->value;
        Frame * inframe = convert_char_to_frame(raw_char_buf);
        
        //Free raw_char_buf
        free(raw_char_buf);
        if(crc16((char*)inframe,MAX_FRAME_SIZE)==0 &&
           inframe->recv_id==receiver->recv_id) {
            fprintf(stderr,"%d recv from %d, data =  %s\n",
                    inframe->recv_id,inframe->send_id,inframe->data);
            SWP* swp = receiver->swp+inframe->send_id;
            int seq = inframe->seq;
            int index = (seq-swp->noackSeq+swp->noackOffset+MAX_SEQ_SIZE)%WINDOWS_SIZE;
            if(swp->ack[index]==0){
                int flag = 0;
                if((swp->noackSeq+WINDOWS_SIZE)>MAX_SEQ_SIZE){
                    if((seq>=swp->noackSeq&&seq<=MAX_SEQ_SIZE-1) ||
                       (0<=seq&&seq<=(swp->noackSeq+WINDOWS_SIZE-1)%MAX_SEQ_SIZE)){
                        flag = 1;
                    }
                }else{
                    if(seq>=swp->noackSeq&&seq<=(swp->noackSeq+WINDOWS_SIZE-1)){
                        flag = 1;
                    }
                }
                if(flag){
                    swp->ack[index]=1;
                    swp->cache[index] = *inframe;
                }
            }
            Frame* outgoing_frame = (Frame*)malloc(MAX_FRAME_SIZE);
            memset(outgoing_frame,0,MAX_FRAME_SIZE);
            outgoing_frame->send_id = receiver->recv_id;
            outgoing_frame->recv_id = inframe->send_id;
            outgoing_frame->seq = inframe->seq;
            uint16_t fcs =  crc16((char*)outgoing_frame,MAX_FRAME_SIZE-2);
            outgoing_frame->FCS[0] = fcs >> 8;
            outgoing_frame->FCS[1] = fcs & 0xFF;
            ll_append_node(outgoing_frames_head_ptr,
                           convert_frame_to_char(outgoing_frame));
            free(outgoing_frame);

            while(swp->ack[(swp->noackSeq)%WINDOWS_SIZE]==1){
                swp->ack[(swp->noackSeq)%WINDOWS_SIZE]=0;
                Frame frame = swp->cache[(swp->noackSeq)%WINDOWS_SIZE];
                if(frame.type==0){
                    printf("<RECV%d>:[%s]\n",
                           receiver->recv_id, swp->cache[(swp->noackSeq)%WINDOWS_SIZE].data);
                }else if(frame.type==1){
                    printf("<RECV%d>:[%s", receiver->recv_id, frame.data);
                }else if(frame.type==2){
                    printf("%s", frame.data);
                }else{
                    printf("%s]\n",frame.data);
                }
                swp->noackSeq = (swp->noackSeq+1)%MAX_SEQ_SIZE;
                swp->noackOffset = swp->noackSeq%WINDOWS_SIZE;
            }
//            printf("<RECV%d>:[%s]\n", receiver->recv_id, inframe->data);
        }

        free(inframe);
        free(ll_inmsg_node);
    }

//    fprintf(stderr,"end handle_incoming_msgs\n");

}

void * run_receiver(void * input_receiver)
{    
    struct timespec   time_spec;
    struct timeval    curr_timeval;
    const int WAIT_SEC_TIME = 0;
    const long WAIT_USEC_TIME = 100000;
    Receiver * receiver = (Receiver *) input_receiver;
    LLnode * outgoing_frames_head;


    //This incomplete receiver thread, at a high level, loops as follows:
    //1. Determine the next time the thread should wake up if there is nothing in the incoming queue(s)
    //2. Grab the mutex protecting the input_msg queue
    //3. Dequeues messages from the input_msg queue and prints them
    //4. Releases the lock
    //5. Sends out any outgoing messages

    pthread_cond_init(&receiver->buffer_cv, NULL);
    pthread_mutex_init(&receiver->buffer_mutex, NULL);

    while(1)
    {    
        //NOTE: Add outgoing messages to the outgoing_frames_head pointer
        outgoing_frames_head = NULL;
        gettimeofday(&curr_timeval, 
                     NULL);

        //Either timeout or get woken up because you've received a datagram
        //NOTE: You don't really need to do anything here, but it might be useful for debugging purposes to have the receivers periodically wakeup and print info
        time_spec.tv_sec  = curr_timeval.tv_sec;
        time_spec.tv_nsec = curr_timeval.tv_usec * 1000;
        time_spec.tv_sec += WAIT_SEC_TIME;
        time_spec.tv_nsec += WAIT_USEC_TIME * 1000;
        if (time_spec.tv_nsec >= 1000000000)
        {
            time_spec.tv_sec++;
            time_spec.tv_nsec -= 1000000000;
        }

        //*****************************************************************************************
        //NOTE: Anything that involves dequeing from the input frames should go 
        //      between the mutex lock and unlock, because other threads CAN/WILL access these structures
        //*****************************************************************************************
        pthread_mutex_lock(&receiver->buffer_mutex);

        //Check whether anything arrived
        int incoming_msgs_length = ll_get_length(receiver->input_framelist_head);
        if (incoming_msgs_length == 0)
        {
            //Nothing has arrived, do a timed wait on the condition variable (which releases the mutex). Again, you don't really need to do the timed wait.
            //A signal on the condition variable will wake up the thread and reacquire the lock
            pthread_cond_timedwait(&receiver->buffer_cv, 
                                   &receiver->buffer_mutex,
                                   &time_spec);
        }

        handle_incoming_msgs(receiver,
                             &outgoing_frames_head);

        pthread_mutex_unlock(&receiver->buffer_mutex);
        
        //CHANGE THIS AT YOUR OWN RISK!
        //Send out all the frames user has appended to the outgoing_frames list
        int ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        while(ll_outgoing_frame_length > 0)
        {
            LLnode * ll_outframe_node = ll_pop_node(&outgoing_frames_head);
            char * char_buf = (char *) ll_outframe_node->value;
            
            //The following function frees the memory for the char_buf object
            send_msg_to_senders(char_buf);

            //Free up the ll_outframe_node
            free(ll_outframe_node);

            ll_outgoing_frame_length = ll_get_length(outgoing_frames_head);
        }
    }
    pthread_exit(NULL);

}
