/* Un mini framework di test. Ogni test gira in un processo figlio (fork): se va in crash (abort di un
 * TODO, stack overflow, errore di AddressSanitizer) o non finisce entro 10 secondi, fallisce solo quel
 * test e gli altri continuano. */

#ifndef TEST_H
#define TEST_H

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int test_failures; /* asserzioni fallite nel test corrente (nel processo figlio) */

#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "    %s:%d: %s\n      (%s)\n", __FILE__, __LINE__, msg,  \
              #cond);                                                          \
      test_failures++;                                                         \
    }                                                                          \
  } while (0)

#define CHECK_EQ(actual, expected, msg)                                        \
  do {                                                                         \
    double a_ = (double)(actual), e_ = (double)(expected);                    \
    if (!(a_ == e_ || (isnan(a_) && isnan(e_)))) {                             \
      fprintf(stderr, "    %s:%d: %s\n      atteso %g, ottenuto %g\n",         \
              __FILE__, __LINE__, msg, e_, a_);                                \
      test_failures++;                                                         \
    }                                                                          \
  } while (0)

/* Confronta i primi n valori di actual con l'array letterale atteso. */
#define CHECK_VALUES(actual, n, msg, ...)                                      \
  do {                                                                         \
    const double exp_[] = {__VA_ARGS__};                                       \
    size_t n_exp_ = sizeof exp_ / sizeof exp_[0];                              \
    int ok_ = (size_t)(n) == n_exp_;                                           \
    for (size_t i_ = 0; ok_ && i_ < n_exp_; i_++)                              \
      ok_ = (actual)[i_] == exp_[i_];                                          \
    if (!ok_) {                                                                \
      fprintf(stderr, "    %s:%d: %s\n      atteso [", __FILE__, __LINE__,     \
              msg);                                                            \
      for (size_t i_ = 0; i_ < n_exp_; i_++)                                   \
        fprintf(stderr, "%s%g", i_ ? ", " : "", exp_[i_]);                     \
      fprintf(stderr, "]\n      ottenuto [");                                  \
      for (size_t i_ = 0; i_ < (size_t)(n); i_++)                              \
        fprintf(stderr, "%s%g", i_ ? ", " : "", (double)(actual)[i_]);         \
      fprintf(stderr, "]\n");                                                  \
      test_failures++;                                                         \
    }                                                                          \
  } while (0)

static int tests_passed, tests_failed;

static void run_test(const char *name, void (*fn)(void)) {
  fflush(stdout);
  pid_t pid = fork();
  if (pid == 0) {
    alarm(10); /* un test che non finisce (es. fibs senza memo) fallisce invece di bloccare tutto */
    test_failures = 0;
    fn();
    fflush(stderr);
    _exit(test_failures ? 1 : 0);
  }
  int status;
  waitpid(pid, &status, 0);
  if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
    printf("  ✔ %s\n", name);
    tests_passed++;
  } else {
    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM)
      printf("  ✖ %s  (tempo scaduto: più di 10 secondi)\n", name);
    else if (WIFSIGNALED(status))
      printf("  ✖ %s  (crash: %s)\n", name, strsignal(WTERMSIG(status)));
    else
      printf("  ✖ %s\n", name);
    tests_failed++;
  }
}

#define RUN(fn, name) run_test(name, fn)

static int test_summary(const char *suite) {
  printf("%s: %d passati, %d falliti\n\n", suite, tests_passed, tests_failed);
  return tests_failed ? 1 : 0;
}

#endif
