#ifndef STU_H
#define STU_H

typedef struct{
	char name[30];
	float score;
}student;

int find_top(student arr[] ,int n);
#endif
