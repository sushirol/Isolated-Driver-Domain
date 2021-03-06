/*
 * mpstat: per-processor statistics
 * (C) 2000-2008 by Sebastien GODARD (sysstat <at> orange.fr)
 *
 ***************************************************************************
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published  by  the *
 * Free Software Foundation; either version 2 of the License, or (at  your *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it  will  be  useful,  but *
 * WITHOUT ANY WARRANTY; without the implied warranty  of  MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License *
 * for more details.                                                       *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA                   *
 ***************************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <sys/utsname.h>

#include "version.h"
#include "mpstat.h"
#include "common.h"
#include "rd_stats.h"

#ifdef USE_NLS
#include <locale.h>
#include <libintl.h>
#define _(string) gettext(string)
#else
#define _(string) (string)
#endif

#define SCCSID "@(#)sysstat-" VERSION ": "  __FILE__ " compiled " __DATE__ " " __TIME__
char *sccsid(void) { return (SCCSID); }

#define comment
unsigned long long uptime[3] = {0, 0, 0};
unsigned long long uptime0[3] = {0, 0, 0};

/* NOTE: Use array of _char_ for bitmaps to avoid endianness problems...*/
unsigned char *cpu_bitmap;	/* Bit 0: Global; Bit 1: 1st proc; etc. */

/* Structures used to store stats */
struct stats_cpu *st_cpu[3];
struct stats_irq *st_irq[3];
struct stats_irqcpu *st_irqcpu[3];

struct tm mp_tstamp[3];

/* Activity flag */
unsigned int actflags = 0;

unsigned int flags = 0;

/* Interval and count parameters */
long interval = -1, count = 0;
int use_inf;

/* Nb of processors on the machine */
int cpu_nr = 0;
/* Nb of interrupts per processor */
int irqcpu_nr = 0;

/*
 ***************************************************************************
 * Print usage and exit
 *
 * IN:
 * @progname	Name of sysstat command
 ***************************************************************************
 */
void usage(char *progname)
{
	fprintf(stderr, _("Usage: %s [ options ] [ <interval> [ <count> ] ]\n"),
		progname);

	fprintf(stderr, _("Options are:\n"
			  "[ -A ] [ -I { SUM | CPU | ALL } ] [ -u ] [ -P { <cpu> | ALL } ] [ -V ]\n"));
	exit(1);
}

/*
 ***************************************************************************
 * SIGALRM signal handler
 *
 * IN:
 * @sig	Signal number. Set to 0 for the first time, then to SIGALRM.
 ***************************************************************************
 */
void alarm_handler(int sig)
{
	signal(SIGALRM, alarm_handler);
	alarm(interval);
}


/* ***************************************************************************
 * Allocate stats structures and cpu bitmap.
 *
 * IN:
 * @nr_cpus	Number of CPUs. This is the real number of available CPUs + 1
 * 		because we also have to allocate a structure for CPU 'all'.
 ***************************************************************************
 */
void salloc_mp_struct(int nr_cpus)
{
	int i;

	for (i = 0; i < 3; i++) {

		if ((st_cpu[i] = (struct stats_cpu *) malloc(STATS_CPU_SIZE * nr_cpus))
		    == NULL) {
			perror("malloc");
			exit(4);
		}
		memset(st_cpu[i], 0, STATS_CPU_SIZE * nr_cpus);

		if ((st_irq[i] = (struct stats_irq *) malloc(STATS_IRQ_SIZE * nr_cpus))
		    == NULL) {
			perror("malloc");
			exit(4);
		}
		memset(st_irq[i], 0, STATS_IRQ_SIZE * nr_cpus);

		if ((st_irqcpu[i] = (struct stats_irqcpu *) malloc(STATS_IRQCPU_SIZE * nr_cpus * irqcpu_nr))
		    == NULL) {
			perror("malloc");
			exit(4);
		}
		memset(st_irqcpu[i], 0, STATS_IRQCPU_SIZE * nr_cpus * irqcpu_nr);

	}

	if ((cpu_bitmap = (unsigned char *) malloc((nr_cpus >> 3) + 1)) == NULL) {
		perror("malloc");
		exit(4);
	}
	memset(cpu_bitmap, 0, (nr_cpus >> 3) + 1);
}

/*
 ***************************************************************************
 * Core function used to display statistics
 *
 * IN:
 * @prev	Position in array where statistics used	as reference are.
 *		Stats used as reference may be the previous ones read, or
 *		the very first ones when calculating the average.
 * @curr	Position in array where statistics for current sample are.
 * @dis		TRUE if a header line must be printed.
 * @prev_string	String displayed at the beginning of a header line. This is
 * 		the timestamp of the previous sample, or "Average" when
 * 		displaying average stats.
 * @curr_string	String displayed at the beginning of current sample stats.
 * 		This is the timestamp of the current sample, or "Average"
 * 		when displaying average stats.
 ***************************************************************************
 */
void write_stats_core(int prev, int curr, int dis,
		      char *prev_string, char *curr_string)
{
	struct stats_cpu *scc, *scp;
	unsigned long long itv, pc_itv, g_itv;
	int cpu;

	/* Test stdout */
	TEST_STDOUT(STDOUT_FILENO);

	/* Compute time interval */
	g_itv = get_interval(uptime[prev], uptime[curr]);

	/* Reduce interval value to one processor */
	if (cpu_nr > 1) {
		itv = get_interval(uptime0[prev], uptime0[curr]);
	}
	else {
		itv = g_itv;
	}

	/* Print CPU stats */
	if (DISPLAY_CPU(actflags)) {
		if (dis) {
			printf("\n%-11s  CPU    %%usr   %%nice    %%sys %%iowait    %%irq   "
			       "%%soft  %%steal  %%guest   %%idle\n",
			       prev_string);
		}

		/* Check if we want global stats among all proc */
		if (*cpu_bitmap & 1) {

			printf("%-11s  all", curr_string);

			printf("  %6.2f  %6.2f  %6.2f  %6.2f  %6.2f  %6.2f  %6.2f  %6.2f  %6.2f\n",
			       ll_sp_value(st_cpu[prev]->cpu_user - st_cpu[prev]->cpu_guest,
					   st_cpu[curr]->cpu_user - st_cpu[curr]->cpu_guest,
					   g_itv),
			       ll_sp_value(st_cpu[prev]->cpu_nice,    st_cpu[curr]->cpu_nice,
					   g_itv),
			       ll_sp_value(st_cpu[prev]->cpu_sys,     st_cpu[curr]->cpu_sys,
					   g_itv),
			       ll_sp_value(st_cpu[prev]->cpu_iowait,  st_cpu[curr]->cpu_iowait,
					   g_itv),
			       ll_sp_value(st_cpu[prev]->cpu_hardirq, st_cpu[curr]->cpu_hardirq,
					   g_itv),
			       ll_sp_value(st_cpu[prev]->cpu_softirq, st_cpu[curr]->cpu_softirq,
					   g_itv),
			       ll_sp_value(st_cpu[prev]->cpu_steal,   st_cpu[curr]->cpu_steal,
					   g_itv),
			       ll_sp_value(st_cpu[prev]->cpu_guest,   st_cpu[curr]->cpu_guest,
					   g_itv),
			       (st_cpu[curr]->cpu_idle < st_cpu[prev]->cpu_idle) ?
			       0.0 :	/* Handle buggy kernels */
			       ll_sp_value(st_cpu[prev]->cpu_idle,    st_cpu[curr]->cpu_idle,
					   g_itv));
		}

		for (cpu = 1; cpu <= cpu_nr; cpu++) {

			scc = st_cpu[curr] + cpu;
			scp = st_cpu[prev] + cpu;

			/* Check if we want stats about this proc */
			if (!(*(cpu_bitmap + (cpu >> 3)) & (1 << (cpu & 0x07))))
				continue;

			printf("%-11s %4d", curr_string, cpu - 1);

			/* Recalculate itv for current proc */
			pc_itv = get_per_cpu_interval(scc, scp);

			if (!pc_itv) {
				/* Current CPU is offline */
				printf("    0.00    0.00    0.00    0.00    0.00    0.00"
				       "    0.00    0.00    0.00\n");
			}
			else {
				printf("  %6.2f  %6.2f  %6.2f  %6.2f  %6.2f  %6.2f"
				       "  %6.2f  %6.2f  %6.2f\n",
				       ll_sp_value(scp->cpu_user - scp->cpu_guest,
						   scc->cpu_user - scc->cpu_guest,
						   pc_itv),
				       ll_sp_value(scp->cpu_nice,    scc->cpu_nice,    pc_itv),
				       ll_sp_value(scp->cpu_sys,     scc->cpu_sys,     pc_itv),
				       ll_sp_value(scp->cpu_iowait,  scc->cpu_iowait,  pc_itv),
				       ll_sp_value(scp->cpu_hardirq, scc->cpu_hardirq, pc_itv),
				       ll_sp_value(scp->cpu_softirq, scc->cpu_softirq, pc_itv),
				       ll_sp_value(scp->cpu_steal,   scc->cpu_steal,   pc_itv),
				       ll_sp_value(scp->cpu_guest,   scc->cpu_guest,   pc_itv),
				       (scc->cpu_idle < scp->cpu_idle) ?
				       0.0 :
				       ll_sp_value(scp->cpu_idle,    scc->cpu_idle,    pc_itv));
			}
		}
	}

	/* Print total number of interrupts per processor */
	if (DISPLAY_IRQ_SUM(actflags)) {
		struct stats_irq *sic, *sip;

		if (dis) {
			printf("\n%-11s  CPU    intr/s\n", prev_string);
		}

		if (*cpu_bitmap & 1) {
			printf("%-11s  all %9.2f\n", curr_string,
			       ll_s_value(st_irq[prev]->irq_nr, st_irq[curr]->irq_nr, itv));
		}

		for (cpu = 1; cpu <= cpu_nr; cpu++) {

			sic = st_irq[curr] + cpu;
			sip = st_irq[prev] + cpu;

			scc = st_cpu[curr] + cpu;
			scp = st_cpu[prev] + cpu;

			/* Check if we want stats about this proc */
			if (!(*(cpu_bitmap + (cpu >> 3)) & (1 << (cpu & 0x07))))
				continue;

			printf("%-11s %4d", curr_string, cpu - 1);

			/* Recalculate itv for current proc */
			pc_itv = get_per_cpu_interval(scc, scp);

			if (!pc_itv) {
				/* Current CPU is offline */
				printf("    0.00\n");
			}
			else {
				printf(" %9.2f\n",
				       ll_s_value(sip->irq_nr, sic->irq_nr, itv));
			}
		}
	}

	if(!use_inf)
	if (DISPLAY_IRQ_CPU(actflags)) {
		int j = 0, offset;
		struct stats_irqcpu *p, *q, *p0, *q0;

		/*
		 * Check if number of interrupts has changed.
		 * NB: A nul interval value indicates that we are
		 * displaying statistics since system startup.
		 */
		if (!dis && interval) {
			do {
				p0 = st_irqcpu[curr] + j;
				if (p0->irq != ~0) {
					q0 = st_irqcpu[prev] + j;
					if (p0->irq != q0->irq)
						j = -2;
				}
				j++;
			}
			while ((j > 0) && (j <= irqcpu_nr));
		}

		if (dis || (j < 0)) {
			/* Print header */
			printf("\n%-11s  CPU", prev_string);
			for (j = 0; j < irqcpu_nr; j++) {
				p0 = st_irqcpu[curr] + j;
				if (p0->irq != ~0)	/* Nb of irq per proc may have varied... */
					printf("  i%03d/s", p0->irq);
			}
			printf("\n");
		}

		for (cpu = 1; cpu <= cpu_nr; cpu++) {

			/*
			 * Check if we want stats about this CPU.
			 * CPU must have been explicitly selected using option -P,
			 * else we display every CPU.
			 */
			if (!(*(cpu_bitmap + (cpu >> 3)) & (1 << (cpu & 0x07))) && USE_P_OPTION(flags))
				continue;

			printf("%-11s  %3d", curr_string, cpu - 1);

			for (j = 0; j < irqcpu_nr; j++) {
				p0 = st_irqcpu[curr] + j;	/* irq field set only for proc #0 */
				/*
				 * A value of ~0 means it is a remaining interrupt
				 * which is no longer used, for example because the
				 * number of interrupts has decreased in /proc/interrupts.
				 */
				if (p0->irq != ~0) {
					q0 = st_irqcpu[prev] + j;
					offset = j;

					/*
					 * If we want stats for the time since system startup,
					 * we have p0->irq != q0->irq, since q0 structure is
					 * completely set to zero.
					 */
					if ((p0->irq != q0->irq) && interval) {
						if (j)
							offset = j - 1;
						q0 = st_irqcpu[prev] + offset;
						if ((p0->irq != q0->irq) && (j + 1 < irqcpu_nr))
							offset = j + 1;
						q0 = st_irqcpu[prev] + offset;
					}
					if ((p0->irq == q0->irq) || !interval) {
						p = st_irqcpu[curr] + (cpu - 1) * irqcpu_nr + j;
						q = st_irqcpu[prev] + (cpu - 1) * irqcpu_nr + offset;
						printf(" %7.2f",
						       S_VALUE(q->interrupt, p->interrupt, itv));
					}
					else
						printf("     N/A");
				}
			}
			printf("\n");
		}
	}
}

/*
 ***************************************************************************
 * Print statistics average
 *
 * IN:
 * @curr	Position in array where statistics for current sample are.
 * @dis		TRUE if a header line must be printed.
 ***************************************************************************
 */
void write_stats_avg(int curr, int dis)
{
	char string[16];

	strncpy(string, _("Average:"), 16);
	string[15] = '\0';
	write_stats_core(2, curr, dis, string, string);
}

/*
 ***************************************************************************
 * Print statistics
 *
 * IN:
 * @curr	Position in array where statistics for current sample are.
 * @dis		TRUE if a header line must be printed.
 ***************************************************************************
 */
void write_stats(int curr, int dis)
{
	char cur_time[2][16];

	/* Get previous timestamp */
	strftime(cur_time[!curr], 16, "%X", &(mp_tstamp[!curr]));

	/* Get current timestamp */
	strftime(cur_time[curr], 16, "%X", &(mp_tstamp[curr]));

	write_stats_core(!curr, curr, dis, cur_time[!curr], cur_time[curr]);
}

/*
 ***************************************************************************
 * Read stats from /proc/interrupts
 *
 * IN:
 * @curr	Position in array where current statistics will be saved.
 ***************************************************************************
 */
void read_interrupts_stat(int curr)
{
	FILE *fp;
	struct stats_irq *st_irq_i;
	struct stats_irqcpu *p;
	static char *line = NULL;
	unsigned long irq = 0;
	unsigned int cpu;
	int cpu_index[cpu_nr], index = 0;
	char *cp, *next;

	for (cpu = 0; cpu < cpu_nr; cpu++) {
		st_irq_i = st_irq[curr] + cpu + 1;
		st_irq_i->irq_nr = 0;
	}

	if ((fp = fopen(INTERRUPTS, "r")) != NULL) {

		if (!line) {
			if ((line = (char *) malloc(INTERRUPTS_LINE + 11 * cpu_nr))
			    == NULL) {
				perror("malloc");
				exit(4);
			}
		}

		/*
		 * Parse header line to see which CPUs are online
		 */
		while (fgets(line, INTERRUPTS_LINE + 11 * cpu_nr, fp) != NULL) {
			next = line;
			while (((cp = strstr(next, "CPU")) != NULL) && (index < cpu_nr)) {
				cpu = strtol(cp + 3, &next, 10);
				cpu_index[index++] = cpu;
			}
			if (index)
				/* Header line found */
				break;
		}

		while ((fgets(line, INTERRUPTS_LINE + 11 * cpu_nr, fp) != NULL) &&
		       (irq < irqcpu_nr)) {

			if (isdigit(line[2])) {

				/* Skip over "<irq>:" */
				if ((cp = strchr(line, ':')) == NULL)
					continue;
				cp++;

				p = st_irqcpu[curr] + irq;
				p->irq = strtol(line, NULL, 10);

				for (cpu = 0; cpu < index; cpu++) {
					p = st_irqcpu[curr] + cpu_index[cpu] * irqcpu_nr + irq;
					st_irq_i = st_irq[curr] + cpu_index[cpu] + 1;
					/*
					 * No need to set (st_irqcpu + cpu * irqcpu_nr)->irq:
					 * same as st_irqcpu->irq.
					 */
					p->interrupt = strtol(cp, &next, 10);
					st_irq_i->irq_nr += p->interrupt;
					cp = next;
				}
				irq++;
			}
		}

		fclose(fp);
	}

	while (irq < irqcpu_nr) {
		/* Nb of interrupts per processor has changed */
		p = st_irqcpu[curr] + irq;
		p->irq = ~0;	/* This value means this is a dummy interrupt */
		irq++;
	}
}

/*
 ***************************************************************************
 * Main loop: read stats from the relevant sources, and display them.
 *
 * IN:
 * @dis_hdr	Set to TRUE if the header line must always be printed.
 * @rows	Number of rows of screen.
 ***************************************************************************
 */
void rw_mpstat_loop(int dis_hdr, int rows)
{
	struct stats_cpu *scc, *scp;
	int cpu;
	int curr = 1, dis = 1;
	unsigned long lines = rows;

	/* Dont buffer data if redirected to a pipe */
	setbuf(stdout, NULL);

	/* Read stats */
	if (cpu_nr > 1) {
		/*
		 * Init uptime0. So if /proc/uptime cannot fill it,
		 * this will be done by /proc/stat.
		 */
		uptime0[0] = 0;
		read_uptime(&(uptime0[0]));
	}
	read_stat_cpu(st_cpu[0], cpu_nr + 1, &(uptime[0]), &(uptime0[0]));

	if (DISPLAY_IRQ_SUM(actflags)) {
		read_stat_irq(st_irq[0], 1);
	}

	if (DISPLAY_IRQ_SUM(actflags) || DISPLAY_IRQ_CPU(actflags)) {
		read_interrupts_stat(0);
	}

	if (!interval) {
		/* Display since boot time */
		mp_tstamp[1] = mp_tstamp[0];
		memset(st_cpu[1], 0, STATS_CPU_SIZE * (cpu_nr + 1));
		memset(st_irq[1], 0, STATS_IRQ_SIZE * (cpu_nr + 1));
		memset(st_irqcpu[1], 0, STATS_IRQCPU_SIZE * (cpu_nr + 1) * irqcpu_nr);
		write_stats(0, DISP_HDR);
		exit(0);
	}

	/* Set a handler for SIGALRM */
	alarm_handler(0);

	/* Save the first stats collected. Will be used to compute the average */
	mp_tstamp[2] = mp_tstamp[0];
	uptime[2] = uptime[0];
	uptime0[2] = uptime0[0];
	memcpy(st_cpu[2], st_cpu[0], STATS_CPU_SIZE * (cpu_nr + 1));
	memcpy(st_irq[2], st_irq[0], STATS_IRQ_SIZE * (cpu_nr + 1));
	memcpy(st_irqcpu[2], st_irqcpu[0], STATS_IRQCPU_SIZE * (cpu_nr + 1) * irqcpu_nr);

	pause();

	do {
		/*
		 * Resetting the structure not needed since every fields will be set.
		 * Exceptions are per-CPU structures: some of them may not be filled
		 * if corresponding processor is disabled (offline).
		 */
		for (cpu = 1; cpu <= cpu_nr; cpu++) {
			scc = st_cpu[curr]  + cpu;
			scp = st_cpu[!curr] + cpu;
			*scc = *scp;
		}

		/* Get time */
		get_localtime(&(mp_tstamp[curr]));

		/* Read stats */
		if (cpu_nr > 1) {
			uptime0[curr] = 0;
			read_uptime(&(uptime0[curr]));
		}
		read_stat_cpu(st_cpu[curr], cpu_nr + 1, &(uptime[curr]), &(uptime0[curr]));

		if (DISPLAY_IRQ_SUM(actflags)) {
			read_stat_irq(st_irq[curr], 1);
		}

		if (DISPLAY_IRQ_SUM(actflags) || DISPLAY_IRQ_CPU(actflags)) {
			read_interrupts_stat(curr);
		}

		/* Write stats */
		if (!dis_hdr) {
			dis = lines / rows;
			if (dis) {
				lines %= rows;
			}
			lines++;
		}
		write_stats(curr, dis);

		if(use_inf)
		{
			printf("\nAverage till sample %ld", count);
			write_stats_avg(curr, dis_hdr);
			printf("---------------------------------------------\n");
			count = count + 1;
		}

		if(!use_inf) {
			if (count > 0) {
				count--;
			}
		}
		if (count) {
			curr ^= 1;
			pause();
		}
	}
	while (count);

	/* Write stats average */
	if(!use_inf)
	write_stats_avg(curr, dis_hdr);
}

/*
 ***************************************************************************
 * Main entry to the program
 ***************************************************************************
 */
int main(int argc, char **argv)
{
	int opt = 0, i;
	struct utsname header;
	int dis_hdr = -1;
	int rows = 23;

#ifdef USE_NLS
	/* Init National Language Support */
	init_nls();
#endif

	/* Get HZ */
	get_HZ();

	/* How many processors on this machine ? */
	cpu_nr = get_cpu_nr(~0);
	/* Calculate number of interrupts per processor */
	irqcpu_nr = get_irqcpu_nr(NR_IRQS, cpu_nr) + NR_IRQCPU_PREALLOC;

	/*
	 * cpu_nr: a value of 2 means there are 2 processors (0 and 1).
	 * In this case, we have to allocate 3 structures: global, proc0 and proc1.
	 */
	salloc_mp_struct(cpu_nr + 1);

	while (++opt < argc) {

		if (!strcmp(argv[opt], "-I")) {
			if (argv[++opt]) {
				if (!strcmp(argv[opt], K_SUM)) {
					/* Display total number of interrupts per CPU */
					actflags |= M_D_IRQ_SUM;
				}
				else if (!strcmp(argv[opt], K_CPU)) {
					/* Display interrupts per CPU */
					actflags |= M_D_IRQ_CPU;
				}
				else if (!strcmp(argv[opt], K_ALL)) {
					actflags |= M_D_IRQ_SUM + M_D_IRQ_CPU;
				}
				else {
					usage(argv[0]);
				}
			}
			else {
				usage(argv[0]);
			}
		}

		else if (!strcmp(argv[opt], "-P")) {
			/* '-P ALL' can be used on UP machines */
			if (argv[++opt]) {
				flags |= F_P_OPTION;
				dis_hdr++;
				if (!strcmp(argv[opt], K_ALL)) {
					if (cpu_nr) {
						dis_hdr = 9;
					}
					/*
					 * Set bit for every processor.
					 * Also indicate to display stats for CPU 'all'.
					 */
					memset(cpu_bitmap, 0xff, ((cpu_nr + 1) >> 3) + 1);
				}
				else {
					if (strspn(argv[opt], DIGITS) != strlen(argv[opt])) {
						usage(argv[0]);
					}
					i = atoi(argv[opt]);	/* Get cpu number */
					if (i >= cpu_nr) {
						fprintf(stderr, _("Not that many processors!\n"));
						exit(1);
					}
					i++;
					*(cpu_bitmap + (i >> 3)) |= 1 << (i & 0x07);
				}
			}
			else {
				usage(argv[0]);
			}
		}

		else if (!strncmp(argv[opt], "-", 1)) {
			for (i = 1; *(argv[opt] + i); i++) {

				switch (*(argv[opt] + i)) {

				case 'A':
					actflags |= M_D_CPU + M_D_IRQ_SUM + M_D_IRQ_CPU;
					/* Select all processors */
					flags |= F_P_OPTION;
					memset(cpu_bitmap, 0xff, ((cpu_nr + 1) >> 3) + 1);
					break;

				case 'u':
					/* Display CPU */
					actflags |= M_D_CPU;
					break;

				case 'V':
					/* Print version number */
					print_version();
					break;

				default:
					usage(argv[0]);
				}
			}
		}

		else if (interval < 0) {
			/* Get interval */
			if (strspn(argv[opt], DIGITS) != strlen(argv[opt])) {
				usage(argv[0]);
			}
			interval = atol(argv[opt]);
			if (interval < 0) {
				usage(argv[0]);
			}
			count = -1;
		}

		else if (count <= 0) {
			/* Get count value */
			if ((strspn(argv[opt], DIGITS) != strlen(argv[opt])) ||
			    !interval) {
				usage(argv[0]);
			}
			count = atol(argv[opt]);
			if(count == 0) {
				printf("Using Infinite mode.\n");
				use_inf = 1;
				count=1;
			}
			if (count < 1) {
				usage(argv[0]);
			}
		}

		else {
			usage(argv[0]);
		}
	}

	/* Default: Display CPU */
	if (!DISPLAY_CPU(actflags) && !DISPLAY_IRQ_SUM(actflags) && !DISPLAY_IRQ_CPU(actflags)) {
		actflags |= M_D_CPU;
	}

	if (count_bits(&actflags, sizeof(unsigned int)) > 1) {
		dis_hdr = 9;
	}
	
	if (!USE_P_OPTION(flags)) {
		/* Option -P not used: set bit 0 (global stats among all proc) */
		*cpu_bitmap = 1;
	}
	if (dis_hdr < 0) {
		dis_hdr = 0;
	}
	if (!dis_hdr) {
		/* Get window size */
		rows = get_win_height();
	}
	if (interval < 0) {
		/* Interval not set => display stats since boot time */
		interval = 0;
	}

	/* Get time */
	get_localtime(&(mp_tstamp[0]));

	/* Get system name, release number and hostname */
	uname(&header);
	print_gal_header(&(mp_tstamp[0]), header.sysname, header.release,
			 header.nodename, header.machine);

	/* Main loop */
	rw_mpstat_loop(dis_hdr, rows);

	return 0;
}
