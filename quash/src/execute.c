/**
 * @file execute.c
 *
 * @brief Implements interface functions between Quash and the environment and
 * functions that interpret an execute commands.
 *
 * @note As you add things to this file you may want to change the method signature
 */

#include "execute.h"
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <deque.h>
#include "quash.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

// Remove this and all expansion calls to it
/**
 * @brief Note calls to any function that requires implementation
 */
#define IMPLEMENT_ME()                                                  \
  fprintf(stderr, "IMPLEMENT ME: %s(line %d): %s()\n", __FILE__, __LINE__, __FUNCTION__)
#define READ_END	0
#define	WRITE_END	1

//Global tracker for active pipes
static int environment_pipes[2][2];
static int prev_pipe = -1;
static int next_pipe = 0;

//Global variable to prevent multiple job queue instantiation
static bool is_job_queue_made = false;


//Set up our queues as needed

IMPLEMENT_DEQUE_STRUCT(pid_queue, pid_t);
IMPLEMENT_DEQUE(pid_queue, pid_t);

//Set up job structure

typedef struct job_struct{
	int job_id;
	pid_queue pids;
	char *cmd;
} job_t;

void pid_wait(pid_queue pid){
	int status;
	waitpid(peek_front_pid_queue(&pid), &status, 0);
	//pop_front_pid_queue(&pid);
}	


IMPLEMENT_DEQUE_STRUCT(job_queue, job_t);
IMPLEMENT_DEQUE(job_queue, job_t);

job_queue jobs;
/***************************************************************************
 * Interface Functions
 ***************************************************************************/

//Worked on By: Brock Sauvage
//Inspired by code from user Jonathan Leffler on Stack Overflow
//Link: stackoverflow.com/298510/how-to-get-the-current-directory-in-a-c-program
// Return a string containing the current working directory.
char* get_current_directory(bool* should_free) {

  // TODO: Get the current working directory. This will fix the prompt path.
  // HINT: This should be pretty simple
  char *pwd;
  // Change this to true if necessary
  *should_free = true;
 	 pwd = getcwd(NULL, 0);
   return (pwd);

}

// Returns the value of an environment variable env_var
const char* lookup_env(const char* env_var) {
  // TODO: Lookup environment variables. This is required for parser to be able
  // to interpret variables from the command line and display the prompt
  // correctly
  // HINT: This should be pretty simple
  // Worked on by: Brock Sauvage
  char *buf;
  buf = getenv(env_var);

  // TODO: Remove warning silencers
  //(void) env_var; // Silence unused variable warning

  return buf;
}

// Check the status of background jobs
void check_jobs_bg_status() {
  // TODO: Check on the statuses of all processes belonging to all background
  // jobs. This function should remove jobs from the jobs queue once all
  // processes belonging to a job have completed.

	for(int i =0; i < length_job_queue(&jobs); i++)
	{
		
		job_t temp_job = pop_front_job_queue(&jobs);
		bool finished = false;
    pid_t temp_pids;
		for(int j = 0; j < length_pid_queue(&temp_job.pids); j++)
		{
			temp_pids = pop_front_pid_queue(&temp_job.pids);
			int status;
			if(waitpid(temp_pids, &status, WNOHANG) == 0)
			{
				finished = true;
			}
			push_back_pid_queue(&temp_job.pids, temp_pids);
    }
		if(finished == false)
		{
			push_back_job_queue(&jobs, temp_job);
		}
		else
		{
		  print_job_bg_complete(temp_job.job_id, temp_pids, temp_job.cmd);
			free(temp_job.cmd);
		}
	}
		
  // TODO: Once jobs are implemented, uncomment and fill the following line

}

// Prints the job id number, the process id of the first process belonging to
// the Job, and the command string associated with this job
void print_job(int job_id, pid_t pid, const char* cmd) {
  printf("[%d]\t%8d\t%s\n", job_id, pid, cmd);
  fflush(stdout);
}

// Prints a start up message for background processes
void print_job_bg_start(int job_id, pid_t pid, const char* cmd) {
  printf("Background job started: ");
  print_job(job_id, pid, cmd);
}

// Prints a completion message followed by the print job
void print_job_bg_complete(int job_id, pid_t pid, const char* cmd) {
  printf("Completed: \t");
  print_job(job_id, pid, cmd);
}

/***************************************************************************
 * Functions to process commands
 ***************************************************************************/
// Run a program reachable by the path environment variable, relative path, or
// absolute path
void run_generic(GenericCommand cmd) {
  // Execute a program with a list of arguments. The `args` array is a NULL
  // terminated (last string is always NULL) list of strings. The first element
  // in the array is the executable
  char* exec = cmd.args[0];
  char** args = cmd.args;

  // TODO: Remove warning silencers
  (void) exec; // Silence unused variable warning
  (void) args; // Silence unused variable warning

  // TODO: Implement run generic
  execvp(args[0], args);

  //perror("ERROR: Failed to execute program");
}

// Print strings
void run_echo(EchoCommand cmd) {
  // Print an array of strings. The args array is a NULL terminated (last
  // string is always NULL) list of strings.
  char** str = cmd.args;

  // TODO: Remove warning silencers
  (void) str; // Silence unused variable warning

  // TODO: Implement echo
  // Being Worked on By: Brock Sauvage
  //for(int i = 0; i < sizeof(str); i++)
	int i = 0;
	while(*(str+i) != NULL)
	{
		printf("%s ", *(str+i));
		i++;
	}
	printf("\n");
  // Flush the buffer before returning
  fflush(stdout);
}

// Sets an environment variable
void run_export(ExportCommand cmd) {
  // Write an environment variable
  const char* env_var = cmd.env_var;
  const char* val = cmd.val;

  // TODO: Remove warning silencers
  (void) env_var; // Silence unused variable warning
  (void) val;     // Silence unused variable warning

  // TODO: Implement export.
  // HINT: This should be quite simple.
	bool flag = 1;
  setenv(env_var, val, flag);
}

// Changes the current working directory
void run_cd(CDCommand cmd) {

  // Get the directory name
  char* dir = cmd.dir;

  // Check if the directory is valid
  if (dir == NULL) {
    perror("ERROR: Failed to resolve path");
    return;
  }
  // Being worked on by Connor Welch
  // TODO: Change directory
  if(chdir(dir) < 0)
	{
		printf("Invalid path.\n");
	}
	else
	{
		// TODO: Update the PWD environment variable to be the new current working
		// directory and optionally update OLD_PWD environment variable to be the old
		// working directory.
		bool flag2 = 1;
		setenv("PWD", dir, flag2);
	}

}

// Sends a signal to all processes contained in a job
void run_kill(KillCommand cmd) {
  int signal = cmd.sig;
  int job_id = cmd.job;

  // TODO: Remove warning silencers
  (void) signal; // Silence unused variable warning
  (void) job_id; // Silence unused variable warning

  // TODO: Kill all processes associated with a background job
  kill(job_id, signal);
}


// Prints the current working directory to stdout
void run_pwd() {
  // TODO: Print the current working directory
  bool isFree = 1;
  fprintf(stdout, "%s\n", get_current_directory(&isFree));

  // Flush the buffer before returning
  fflush(stdout);
}

// Prints all background jobs currently in the job list to stdout
void run_jobs() {
  // TODO: Print background jobs
	job_t temp_job;
	char *command;
  for(int i = 0; i < length_job_queue(&jobs); i++)
	{
			temp_job = pop_front_job_queue(&jobs);
			command = temp_job.cmd;
			print_job(temp_job.job_id, peek_front_pid_queue(&temp_job.pids), command);
			push_back_job_queue(&jobs, temp_job);
	}

  // Flush the buffer before returning
  fflush(stdout);
}

/***************************************************************************
 * Functions for command resolution and process setup
 ***************************************************************************/

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for child processes.
 *
 * This version of the function is tailored to commands that should be run in
 * the child process of a fork.
 *
 * @param cmd The Comma3_bg
t3_cat
t3_find_grep
t3_jobs
t3_knd to try to run
 *
 * @sa Command
 */
void child_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
  case GENERIC:
    run_generic(cmd.generic);
    break;

  case ECHO:
    run_echo(cmd.echo);
    break;

  case PWD:
    run_pwd();
    break;

  case JOBS:
    run_jobs();
    break;

  case EXPORT:
  case CD:
  case KILL:
  case EXIT:
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for the quash process.
 *
 * This version of the function is tailored to commands that should be run in
 * the parent process (quash).
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void parent_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
  case EXPORT:
    run_export(cmd.export);
    break;

  case CD:
    run_cd(cmd.cd);
    break;

  case KILL:
    run_kill(cmd.kill);
    break;

  case GENERIC:
  case ECHO:
  case PWD:
  case JOBS:
  case EXIT:
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}


/**
 * @brief Creates one new process centered around the @a Command in the @a
 * CommandHolder setting up redirects and pipes where needed
 *
 * @note Processes are not the same as jobs. A single job can have multiple
 * processes running under it. This function creates a process that is part of a
 * larger job.
 *
 * @note Not all commands should be run in the child process. A few need to
 * change the quash process in some way
 *
 * @param holder The CommandHolder to try to run
 *
 * @sa Command CommandHolder
 */
void create_process(CommandHolder holder, pid_queue *pids) {

  // Read the flags field from the parser
  bool p_in  = holder.flags & PIPE_IN;
  bool p_out = holder.flags & PIPE_OUT;
  bool r_in  = holder.flags & REDIRECT_IN;
  bool r_out = holder.flags & REDIRECT_OUT;
  bool r_app = holder.flags & REDIRECT_APPEND; // This can only be true if r_out
                                               // is true

  // TODO: Remove warning silencers
  //(void) p_in;  // Silence unused variable warning
  //(void) p_out; // Silence unused variable warning
  //(void) r_in;  // Silence unused variable warning
  //(void) r_out; // Silence unused variable warning
  //(void) r_app; // Silence unused variable warning

  // TODO: Setup pipes, redirects, and new process
  pid_t pid;

  if(p_out)
  {
 		pipe(environment_pipes[next_pipe]);
  }

  pid = fork();

  if(0 == pid)
  {
		
		if(r_in)
		{
			int in_file = 0;
			in_file = open(holder.redirect_in, O_RDONLY | O_CLOEXEC);
			
			if(r_app)
			{
				int out_file = open(holder.redirect_out, O_APPEND | O_WRONLY);
				dup2(in_file, STDIN_FILENO);
				dup2(out_file, STDOUT_FILENO);
				close(in_file);
				close(out_file);
			}
			else if(r_out)
			{
				int out_file = open(holder.redirect_out, O_WRONLY | O_TRUNC);
				dup2(in_file, STDIN_FILENO);
				dup2(out_file, STDOUT_FILENO);
				close(in_file);
				close(out_file);
			}
		}
		if(p_in)
		{
			dup2(environment_pipes[prev_pipe][READ_END], STDIN_FILENO);
			close(environment_pipes[prev_pipe][WRITE_END]);
		}
		if(p_out)
		{
			dup2(environment_pipes[prev_pipe][READ_END], STDOUT_FILENO);
			close(environment_pipes[next_pipe][READ_END]);
		}
		child_run_command(holder.cmd);
		exit(EXIT_SUCCESS);
	}
	else
	{
		if(p_in)
		{
			close(environment_pipes[next_pipe][WRITE_END]);
		}
	
		next_pipe = (next_pipe + 1) % 2;
		prev_pipe = (prev_pipe + 1) % 2;
		push_back_pid_queue(pids, pid);
		parent_run_command(holder.cmd);
  }
  
  return;
  
  IMPLEMENT_ME();
}

// Run a list of commands
void run_script(CommandHolder* holders) {

  if (holders == NULL)
    return;

	if(!is_job_queue_made)
	{
		jobs = new_job_queue(1);
  	is_job_queue_made = true;
	}



  //check_jobs_bg_status();

  if (get_command_holder_type(holders[0]) == EXIT &&                                   
      get_command_holder_type(holders[1]) == EOC) {
    end_main_loop();
    return;
  }
	job_t curr_job;
	CommandType type;
  curr_job.job_id = length_job_queue(&jobs)+1;
  curr_job.pids = new_pid_queue(1);
  curr_job.cmd = get_command_string();

  // Run all commands in the `holder` array

  for (int i = 0; (type = get_command_holder_type(holders[i])) != EOC; ++i)
    create_process(holders[i], &curr_job.pids);
 

  if (!(holders[0].flags & BACKGROUND)) {
    // Not a background Job
    // TODO: Wait for all processes under the job to complete
    //keep track of pid and wait to finish
		
    //apply_pid_queue(&curr_job.pids, pid_wait);
		int status;
		while(!is_empty_pid_queue(&curr_job.pids))
		{
				pid_t temp_pid = pop_front_pid_queue(&curr_job.pids);
				waitpid(temp_pid, &status, 0);
		}
  

  }
  else {
    // A background job.
    // TODO: Push the new job to the job queue
		push_back_job_queue(&jobs, curr_job);
		char* cmd =curr_job.cmd;
		int job_id = curr_job.job_id;
		pid_t pid = peek_front_pid_queue(&curr_job.pids);
    // TODO: Once jobs are implemented, uncomment and fill the following line
    print_job_bg_start(job_id, pid, cmd);
  }

	  destroy_pid_queue(&curr_job.pids);
	//free(curr_job.cmd);
	//free(&jobs);
}
