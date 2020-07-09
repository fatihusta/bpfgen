#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>

#include "managers/imrManager.h"

FILE *logger = NULL; //Bpfgen logger
/*
	Read in the bpf configuration file, translate it to the IMR, and load the IMR BPF program. 
	@param run_bootstrap - if bootstrap tests should be run, passed to ruleset read 
	@param test_to_run - if bootstrap tests are run, which test is run. Passed to ruleset read
	@param debug - bool for if debug information is on
	@return The return code after doing translation to IMR and loading the BPF program
*/
static int sdwan2bpf(int run_bootstrap, int test_to_run, bool debug)
{
	//Initialize variables 
	json_t *bpf_settings;
	struct imr_state *state;
	int ret;

	//Read in /etc/config/bpf.json which describes rules to translates 
	bpf_settings = read_bpf_file();
	if (bpf_settings == NULL) 
	{
		return -1;
	}

	//Read in ruleset from the file descriptor 
	state = imr_ruleset_read(bpf_settings, run_bootstrap, test_to_run, debug);
	if (state == NULL) {
		fprintf(logger, "Ruleset read failed\n");
		json_decref(bpf_settings);
		return -1;
	}

	//Release json object that is no longer needed
	json_decref(bpf_settings);

	//Translate IMR to BPF and load BPF program
	ret = imr_do_bpf(state, debug);

	//Free memory
	imr_state_free(state);

	return ret;
}

//Main function
int main(int argc, char *argv[])
{
	int run_bootstrap = 0;
	int test_to_run = 0;
	bool debug = false;

	//Run getopt if arguments passed to bpfgen
	if (argc >= 2) {
		int opt;
		//Look for if -t is passed for bootstrap and test_to_run
		while ((opt = getopt(argc, argv, "dt:")) != -1) {
			switch(opt){
				case 't':
					//Bootstrap and test_to_run passed
					run_bootstrap = 1;
					test_to_run = atoi(optarg);
					break;
				case 'd':
					debug = true;
					break;
				default:
					fprintf(stderr, "Not sure what you're looking for there, sir\n");
					break;
			}
		}
	}	

	//Logging
	if (debug) {
		//If debugging open a file 
		logger = fopen("/tmp/bpfgen.log", "w");

		//If file open fails use stderr
		if(!logger) {
			fprintf(stderr, "Using stdout in debug mode\n");
			logger = stderr;
		}
	}
	else {
		//Normal logging is to stderr
		logger = stderr;
	}

	//Main function to translate and load bpf program
	int ret; // return code
	ret = sdwan2bpf(run_bootstrap, test_to_run, debug);

	//free logger memory if in debug mode
	if (debug) 
		fclose (logger);

	return ret;
}
