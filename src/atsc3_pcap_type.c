/*
 * atsc3_pcap_type.c
 *
 *  Created on: Oct 9, 2019
 *      Author: jjustman
 */


#include <stdio.h>
#include <string.h>
#include "atsc3_pcap_type.h"


int _ATSC3_PCAP_TYPE_DEBUG_ENABLED = 1;
int _ATSC3_PCAP_TYPE_TRACE_ENABLED = 0;

/**
 *
 * either:
 * fseek(f, 0, SEEK_END); // seek to end of file
size = ftell(f); // get current file pointer
fseek(f, 0, SEEK_SET); // seek back to beginning of file
// proceed with allocating memory and reading the file
 *
 * or
 *
 * #include <sys/stat.h>
struct stat st;
stat(filename, &st);
size = st.st_size;


 */

atsc3_pcap_replay_context_t* atsc3_pcap_replay_open_filename(const char* pcap_filename) {
	atsc3_pcap_replay_context_t* atsc3_pcap_replay_context = calloc(1, sizeof(atsc3_pcap_replay_context_t));
	atsc3_pcap_replay_context->pcap_file_name = calloc(strlen(pcap_filename) + 1, sizeof(char));
	strncpy(atsc3_pcap_replay_context->pcap_file_name, pcap_filename, strlen(pcap_filename));

	struct stat st;
	stat(atsc3_pcap_replay_context->pcap_file_name, &st);
	if(!st.st_size) {
		return NULL;
	}

	atsc3_pcap_replay_context->pcap_file_len = st.st_size;
	atsc3_pcap_replay_context->pcap_file_pos = 0;

	atsc3_pcap_replay_context->pcap_fp = fopen(atsc3_pcap_replay_context->pcap_file_name, "r");
	if(!atsc3_pcap_replay_context->pcap_fp) {
		return NULL;
	}

	return atsc3_pcap_replay_context;
}

//used for inclusion of pcap's via android assetManager
atsc3_pcap_replay_context_t* atsc3_pcap_replay_open_from_fd(const char* pcap_filename, int pcap_fd, long pcap_start, long pcap_length) {
    if(pcap_fd < 0 ) {
		printf("atsc3_pcap_replay_open_from_fd: ERROR: pcap_fd is %d", pcap_fd);

		return NULL;
    }

    FILE* pcap_fp = fdopen(pcap_fd, "r");

    if(!pcap_fp) {
        return NULL;
    }

    atsc3_pcap_replay_context_t* atsc3_pcap_replay_context = calloc(1, sizeof(atsc3_pcap_replay_context_t));
    atsc3_pcap_replay_context->pcap_file_name = calloc(strlen(pcap_filename) + 1, sizeof(char));
    strncpy(atsc3_pcap_replay_context->pcap_file_name, pcap_filename, strlen(pcap_filename));

    //embedded android assets start in the an internal offset from AAsset_openFileDescriptor
	fseek(pcap_fp, pcap_start, SEEK_SET);
    atsc3_pcap_replay_context->pcap_fd_start = pcap_start;
	atsc3_pcap_replay_context->pcap_file_len = pcap_length;
    atsc3_pcap_replay_context->pcap_file_pos = 0;

    atsc3_pcap_replay_context->pcap_fp = pcap_fp;
    long my_pcap_fp_pos = ftell(pcap_fp);
    printf("atsc3_pcap_replay_open_from_fd: fd: %d, pos: %lu", pcap_fd, my_pcap_fp_pos);

    return atsc3_pcap_replay_context;
}

atsc3_pcap_replay_context_t* atsc3_pcap_replay_iterate_packet(atsc3_pcap_replay_context_t* atsc3_pcap_replay_context_to_iterate) {
	if(!atsc3_pcap_replay_context_to_iterate->first_wallclock_timeval.tv_sec && !atsc3_pcap_replay_context_to_iterate->first_wallclock_timeval.tv_usec) {
		gettimeofday(&atsc3_pcap_replay_context_to_iterate->first_wallclock_timeval, NULL);
	}

	if(atsc3_pcap_replay_context_to_iterate->pcap_file_pos + ATSC3_PCAP_MIN_GLOBAL_AND_PACKET_AND_ETH_HEADER_LENGTH > atsc3_pcap_replay_context_to_iterate->pcap_file_len) {
		return NULL;
	}

	if(!atsc3_pcap_replay_context_to_iterate->pcap_fp) {
		return NULL;
	}

	if(atsc3_pcap_replay_context_to_iterate->atsc3_pcap_packet_instance.current_pcap_packet) {
		//_ATSC3_PCAP_TYPE_DEBUG("block_Destroy on packet: %p", atsc3_pcap_replay_context_to_iterate->atsc3_pcap_packet_instance.current_pcap_packet);
		block_Destroy(&atsc3_pcap_replay_context_to_iterate->atsc3_pcap_packet_instance.current_pcap_packet);
	}
    
    //jjustman-2019-10-11 - clear our our last packet header, but not not overwrite block_t* ptr to packet payload
	memset(&atsc3_pcap_replay_context_to_iterate->atsc3_pcap_packet_instance.atsc3_pcap_packet_header, 0, ATSC3_PCAP_PACKET_HEADER_SIZE_BYTES);

	//read our global header first
	if(!atsc3_pcap_replay_context_to_iterate->has_read_atsc3_pcap_global_header) {
		fread((void*)&atsc3_pcap_replay_context_to_iterate->atsc3_pcap_global_header, ATSC3_PCAP_GLOBAL_HEADER_SIZE_BYTES, 1, atsc3_pcap_replay_context_to_iterate->pcap_fp);
		atsc3_pcap_replay_context_to_iterate->has_read_atsc3_pcap_global_header = true;
	}
    //sizeof(atsc3_pcap_packet_header_t) ->
	fread((void*)&atsc3_pcap_replay_context_to_iterate->atsc3_pcap_packet_instance.atsc3_pcap_packet_header, ATSC3_PCAP_PACKET_HEADER_SIZE_BYTES, 1, atsc3_pcap_replay_context_to_iterate->pcap_fp);

	//keep track of our "last" packet ts
	atsc3_pcap_replay_context_to_iterate->last_packet_ts_sec = atsc3_pcap_replay_context_to_iterate->current_packet_ts_sec;
	atsc3_pcap_replay_context_to_iterate->last_packet_ts_usec = atsc3_pcap_replay_context_to_iterate->current_packet_ts_usec;

	//assign our "current" packet ts
	atsc3_pcap_replay_context_to_iterate->current_packet_ts_sec = atsc3_pcap_replay_context_to_iterate->atsc3_pcap_packet_instance.atsc3_pcap_packet_header.ts_sec;
	atsc3_pcap_replay_context_to_iterate->current_packet_ts_usec = atsc3_pcap_replay_context_to_iterate->atsc3_pcap_packet_instance.atsc3_pcap_packet_header.ts_usec;

	if(!atsc3_pcap_replay_context_to_iterate->first_packet_ts_timeval.tv_sec && !atsc3_pcap_replay_context_to_iterate->first_packet_ts_timeval.tv_usec) {
		atsc3_pcap_replay_context_to_iterate->first_packet_ts_timeval.tv_sec = atsc3_pcap_replay_context_to_iterate->current_packet_ts_sec;
		atsc3_pcap_replay_context_to_iterate->first_packet_ts_timeval.tv_usec = atsc3_pcap_replay_context_to_iterate->current_packet_ts_usec;
	}

	atsc3_pcap_replay_context_to_iterate->pcap_file_pos += sizeof(atsc3_pcap_global_header_t) + sizeof(atsc3_pcap_packet_header_t);

	atsc3_pcap_replay_context_to_iterate->atsc3_pcap_packet_instance.current_pcap_packet = block_Alloc(atsc3_pcap_replay_context_to_iterate->atsc3_pcap_packet_instance.atsc3_pcap_packet_header.incl_len);

	_ATSC3_PCAP_TYPE_DEBUG("PEEK: Reading packet: %d, size: %d, fpos is: %ld, next emission: ts_sec: %u, ts_usec: %u",
                           atsc3_pcap_replay_context_to_iterate->pcap_read_packet_count,
                           atsc3_pcap_replay_context_to_iterate->atsc3_pcap_packet_instance.atsc3_pcap_packet_header.incl_len,
                           ftell(atsc3_pcap_replay_context_to_iterate->pcap_fp),
                           atsc3_pcap_replay_context_to_iterate->atsc3_pcap_packet_instance.atsc3_pcap_packet_header.ts_sec,
                           atsc3_pcap_replay_context_to_iterate->atsc3_pcap_packet_instance.atsc3_pcap_packet_header.ts_usec);

	fread((void*)atsc3_pcap_replay_context_to_iterate->atsc3_pcap_packet_instance.current_pcap_packet->p_buffer, atsc3_pcap_replay_context_to_iterate->atsc3_pcap_packet_instance.atsc3_pcap_packet_header.incl_len, 1, atsc3_pcap_replay_context_to_iterate->pcap_fp);

	atsc3_pcap_replay_context_to_iterate->pcap_file_pos += atsc3_pcap_replay_context_to_iterate->atsc3_pcap_packet_instance.atsc3_pcap_packet_header.incl_len;
	atsc3_pcap_replay_context_to_iterate->pcap_read_packet_count++;

	return atsc3_pcap_replay_context_to_iterate;
}


atsc3_pcap_replay_context_t* atsc3_pcap_replay_usleep_packet(atsc3_pcap_replay_context_t* atsc3_pcap_replay_context_to_iterate) {

	if(atsc3_pcap_replay_context_to_iterate->last_packet_ts_sec || atsc3_pcap_replay_context_to_iterate->last_packet_ts_usec) {


		struct timeval last_packet_timeval;
		last_packet_timeval.tv_sec = atsc3_pcap_replay_context_to_iterate->last_packet_ts_sec;
		last_packet_timeval.tv_usec = atsc3_pcap_replay_context_to_iterate->last_packet_ts_usec;

		struct timeval current_packet_timeval;
		current_packet_timeval.tv_sec = atsc3_pcap_replay_context_to_iterate->current_packet_ts_sec;
		current_packet_timeval.tv_usec = atsc3_pcap_replay_context_to_iterate->current_packet_ts_usec;

		//compute our service time internal wall-clock differential for skew based upon our first wallclock timeval and first_packet_ts_timeval
		//against current_wallclock and current packet ts
		//should have used pointers...:/

		long long wallclock_runtime_packet_capture_ts_differentialUS = 0;
        gettimeofday(&atsc3_pcap_replay_context_to_iterate->current_wallclock_timeval, NULL);

        if((atsc3_pcap_replay_context_to_iterate->first_wallclock_timeval.tv_sec || atsc3_pcap_replay_context_to_iterate->first_wallclock_timeval.tv_usec) &&
			(atsc3_pcap_replay_context_to_iterate->first_packet_ts_timeval.tv_sec || atsc3_pcap_replay_context_to_iterate->first_packet_ts_timeval.tv_usec) &&
			(last_packet_timeval.tv_sec || last_packet_timeval.tv_usec)) {

			long long wallclock_runtime_us = timediff(atsc3_pcap_replay_context_to_iterate->current_wallclock_timeval, atsc3_pcap_replay_context_to_iterate->first_wallclock_timeval);
			long long packet_runtime_us = timediff(current_packet_timeval, atsc3_pcap_replay_context_to_iterate->first_packet_ts_timeval);

			wallclock_runtime_packet_capture_ts_differentialUS = packet_runtime_us - wallclock_runtime_us;
		}

		if(wallclock_runtime_packet_capture_ts_differentialUS > 1) {
            _ATSC3_PCAP_TYPE_INFO("pcap timing information: current packet timeval: s.us: %ld.%ld, last packet timeval: s.us: %ld.%ld, target sleep duration uS: %lld",
                  current_packet_timeval.tv_sec,
                  current_packet_timeval.tv_usec,
                  last_packet_timeval.tv_sec,
                  last_packet_timeval.tv_usec,
                  wallclock_runtime_packet_capture_ts_differentialUS);

            //nanosleep intead of usleep(packet_capture_ts_differentialUS);
            struct timespec rqtp, rmtp;

            rqtp.tv_sec = wallclock_runtime_packet_capture_ts_differentialUS / 1000000;
            rqtp.tv_nsec = (wallclock_runtime_packet_capture_ts_differentialUS % 1000000) * 1000;

            _ATSC3_PCAP_TYPE_INFO("setting nanosleep to s: %ld, ns: %ld", rqtp.tv_sec, rqtp.tv_nsec);
            int ret = nanosleep(&rqtp, &rmtp);

            if(ret != 0) {
                //signal interruption,
                _ATSC3_PCAP_TYPE_INFO("nanosleep returned: %d, sleep duration actual: s: %ld, ns: %ld", ret, rmtp.tv_sec, rmtp.tv_nsec);
            }
        } else {
            //falling behind
            _ATSC3_PCAP_TYPE_WARN("pcap timing falling behind: current packet timeval: s.us: %ld.%ld  last packet timeval: s.us: %ld.%ld  wallclock_runtime_packet_capture_ts_differentialUS: %lld, sleep would be negative!",
                    current_packet_timeval.tv_sec,
                    current_packet_timeval.tv_usec,
                    last_packet_timeval.tv_sec,
                    last_packet_timeval.tv_usec,
                    wallclock_runtime_packet_capture_ts_differentialUS);
            //don't update atsc3_pcap_replay_context_to_iterate->current_wallclock_timeval as we are still behind...
        }

		gettimeofday(&atsc3_pcap_replay_context_to_iterate->current_wallclock_timeval, NULL);
		atsc3_pcap_replay_context_to_iterate->last_wallclock_timeval.tv_sec = atsc3_pcap_replay_context_to_iterate->current_wallclock_timeval.tv_sec;
		atsc3_pcap_replay_context_to_iterate->last_wallclock_timeval.tv_usec = atsc3_pcap_replay_context_to_iterate->current_wallclock_timeval.tv_usec;

	}


    return atsc3_pcap_replay_context_to_iterate;
}
