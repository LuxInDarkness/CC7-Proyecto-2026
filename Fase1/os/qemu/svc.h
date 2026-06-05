#ifndef SVC_H
#define SVC_H

int read_svc_sp(void);
void write_svc_sp(int sp);
void write_svc_sp_from_svc(int sp);
int read_spsr_svc(void);
void write_usr_sp(int sp);
int read_usr_sp(void);

#endif // SVC_H