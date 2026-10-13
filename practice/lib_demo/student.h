#ifndef STUDENT_H
#define STUDENT_H

#define ID_LEN 8
#define NAME_LEN 16
#define SCORE_LEN 4
#define MAX_LEN (ID_LEN+NAME_LEN+SCORE_LEN)

int modify_score(const char *path,int line ,int newscore);

#endif