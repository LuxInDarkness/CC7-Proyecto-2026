#ifndef SVC_H
#define SVC_H

int read_svc_sp(void);
void write_svc_sp(int sp);
void write_svc_sp_from_svc(int sp);
int read_spsr_svc(void);

/* Read/write the actual USR-mode stack pointer from any privileged mode.
 * Temporarily switches to USR mode, reads/writes SP, then switches back. */
int read_usr_sp(void);
void write_usr_sp(int sp);

#endif // SVC_H