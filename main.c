#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <editline/readline.h>
#include "mpc.h"

// State enumeration.
enum{NORM, TEST, HELP};

long eval_op(long x, char* op, long y){
  if(strcmp(op, "+") == 0) { return x + y; }
  if(strcmp(op, "-") == 0) { return x - y; }
  if(strcmp(op, "*") == 0) { return x * y; }
  if(strcmp(op, "/") == 0) { return x / y; }
  return 0;
}

long eval(mpc_ast_t* t, FILE *fp){

  fp = fopen("log", "a");
  // If tagged as number return it directly.
  if(strstr(t->tag, "number")){
    //fprintf(fp, "%p Tag number: %i\n",&t->contents , atoi(t->contents));
    return atoi(t->contents);
  }

  // The operator is always second child.
  char* op = t->children[1]->contents;
  fprintf(fp, "%p \tOperator: %s\n", &t->children[1]->contents, t->children[1]->contents);
  // We store the third child in 'x'
  long x = eval(t->children[2], fp);
  fprintf(fp, "%p \tChild stored in x: %ld\n", &t->children[2], eval(t->children[2], fp));

  // Iterate the remaining children and combining.
  int i = 3;
  while(strstr(t->children[i]->tag, "expr")){
    x = eval_op(x, op, eval(t->children[i], fp));
    fprintf(fp, "%p \tIteration of x: %ld\n", &x, x);
    i++;
  }
  
  fclose(fp);

  return x;
}

int main(int argc, char **argv) {

  // Initializes default state
  int state = NORM;
  
  // Change state if correct flag is given.
  if(argc > 1){
    if(0 == strcmp(argv[1], "--test")) state = TEST;
    if(0 != strcmp(argv[1], "--test")) state = HELP;
  }

  FILE *fp;
  
  // Help triggered by wrong arguments.
  if(state == HELP){
    char* initial_string = "\n*** Program running state: help ***\n\n";
    fp = fopen("log", "w");
    fprintf(fp, "%p\n", &fp);
    fprintf(fp, "%sNumber of arguments: %i\n", initial_string, argc);
    for(int i = 0; i < argc; i++)
      fprintf(fp, "Argument %i: %s\n", i, argv[i]);
    fclose(fp);
    puts("To run test file use --test test_file.");
    puts("To run normaly supply no arguments.");
    return 1;
  }

  // MPC parser Initialization.
  // Parser Creation.
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");
  
  // Language definition.
  mpca_lang(MPCA_LANG_DEFAULT,
	    "					                \
	    number   : /-?[0-9]+/ ;				\
            operator : '+' | '-' | '*' | '/' ;			\
            expr     : <number> | '(' <operator> <expr>+ ')' ;	\
            lispy    : /^/ <operator> <expr>+ /$/ ;		\
	    ",
	    Number, Operator, Expr, Lispy);

  // Create log file for normal state.
  if(state == NORM){
    char* initial_string = "\n*** Program running state: norm ***\n\n";
    fp = fopen("log", "w");
    fprintf(fp, "%p\n", &fp);
    puts("crypt-lisp");
    puts("Press Ctrl+c to Exit\n");
    fprintf(fp, "%s", initial_string);
    fclose(fp);
  }

  // Create log file for test state.
  if(state == TEST){
    char* initial_string = "\n*** Program running state: test ***\n\n";
    fp = fopen("log", "w");
    fprintf(fp, "%p\n", &fp);
    fprintf(fp, "%s", initial_string);
    fprintf(fp, "START***********************\n");
    fprintf(fp, "Test file given: %s\n\n", argv[2]);
    fclose(fp);
  }


  // Track user input.
  unsigned int input_count = 0;
  
  while(1){

    // Runs test state if flag is given.
    if(state == TEST){
      char test_file[256];
      memset(test_file, '\0', sizeof(test_file));
      if(sizeof(argv[2]) < sizeof(test_file)) { strncpy(test_file, argv[2], sizeof(test_file)); }
      else{
	puts("Name of file is too large...");
	break;
      }

      printf("Checking for test file: %s\n", test_file);

      if(access(test_file, F_OK) == 0){
	fp = fopen(test_file, "r");

	char test_line[256];
	fgets(test_line, sizeof(test_line), fp);
	printf("Input: %s", test_line);
	fclose(fp);

	fp = fopen("log", "a");
	fprintf(fp, "%p: x0\n\t\t\tInput: %s\n", &test_line, test_line);
	fclose(fp);

	mpc_result_t r;
	if(mpc_parse("<stdin>", test_line, Lispy, &r)){
	  
	  fp = fopen("log", "a");
	  //fprintf(fp, "%p: x0\n\t\t\tInput: %s\n", &test_line, test_line);
	  fprintf(fp, "%p: x0\n\t\t\tOutput:\n", &r);
	  //mpc_ast_print_to(r.output, fp);
	  long result = eval(r.output, fp);
	  fprintf(fp, "%p \t%li\n", &result, result);
	  fprintf(fp, "END*************************\n");
	  fclose(fp);
	
	  mpc_ast_delete(r.output);
	
	} else {
	  mpc_err_print(r.error);
	  
	  fp = fopen("log", "a");
	  fprintf(fp, "%p: x0\n\t\t\tInput: %s\n", &test_line, test_line);
	  fprintf(fp, "%p: x0\n\t\t\tOutput:\n", &r);
	  fprintf(fp, "START***********************\n");
	  mpc_err_print_to(r.error, fp);
	  fprintf(fp, "END*************************\n");
	  fclose(fp);
	  
	  mpc_err_delete(r.error);

	}
	break;
      }
    }
    
    // Default Normal state.
    else {
      
      char* input = readline("crypt-lisp>>> ");
      add_history(input);

      fp = fopen("log", "a");
      fprintf(fp, "START***********************\n");
      fprintf(fp, "%p: x%i\n\t\t\tInput: %s\n", &input_count, input_count, input);
      fclose(fp);

      mpc_result_t r;
      if(mpc_parse("<stdin>", input, Lispy, &r)){
	// Prints AST if successful
	long result = eval(r.output, fp);
	printf("%li\n", result);
	
	fp = fopen("log", "a");
	fprintf(fp, "\n\t\t\tOutput:\n");
	fprintf(fp, "%p %li\n", &result, result);
	fprintf(fp, "END*************************\n");
	fclose(fp);
	
	mpc_ast_delete(r.output);
      } else {
	mpc_err_print(r.error);
	
	fp = fopen("log", "a");
	fprintf(fp, "\n\t\t\tOutput:\n");
	fprintf(fp, "START***********************\n");
	mpc_err_print_to(r.error, fp);
	fprintf(fp, "END*************************\n");
	fclose(fp);

	mpc_err_delete(r.error);
      }
      input_count++;
      
      free(input);
    }
  }

  // Clean up mpc as name says.
  mpc_cleanup(4, Number, Operator, Expr, Lispy);
  
  return 0;
}
