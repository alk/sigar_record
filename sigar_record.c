#include <stdio.h>
#include <stdlib.h>
#include <sigar.h>
#include <memory.h>
#include <time.h>
#include <stdint.h>
#ifndef _WIN32
/* for sleep */
#include <unistd.h>
#else

#include <windows.h>

#include <io.h>
#include <fcntl.h>

#define sleep(a) Sleep((a) * 1000)
#endif

struct layer {
	int cap, size;
	char *buf;
};

static int now_layer;
static struct layer layers[16];

static
struct layer *current_layer_fun(void)
{
	return layers + now_layer;
}

static
char *ensure_space(int amount)
{
	struct layer *l = current_layer_fun();
	int new_cap;

	if (l->size + amount <= l->cap) {
		return l->buf + l->size;
	}

	new_cap = l->cap * 3 / 2;
	if (l->size + amount > new_cap) {
		new_cap = (l->size + amount) * 3 / 2;
	}

	l->buf = realloc(l->buf, new_cap);
	l->cap = new_cap;

	return l->buf + l->size;
}

static
void eat_space(int amount)
{
	struct layer *l = current_layer_fun();
	l->size += amount;
}

static
void push_layer(void)
{
	struct layer *l;
	now_layer++;
	l = current_layer_fun();
	l->size = 0;
}

static
void write_stuff(const void *from, size_t _len)
{
	uint32_t len = (uint32_t)_len;
	size_t eaten = len + sizeof(len);
	char *space = ensure_space(eaten);
	memcpy(space, &len, sizeof(len));
	memcpy(space + sizeof(len), from, len);
	eat_space(eaten);
}

static
void pop_layer(void)
{
	struct layer *old_l = current_layer_fun();
	--now_layer;

	write_stuff(old_l->buf, old_l->size);

	if (now_layer == 0) {
		struct layer *l = current_layer_fun();
		fwrite(l->buf, 1, l->size, stdout);
		l->size = 0;
	}
}

sigar_t *sigar;

static
void write_sample(void)
{
	time_t now;
	sigar_proc_list_t proc_list;
	sigar_proc_mem_t proc_mem;
	sigar_proc_cpu_t proc_cpu;
	sigar_proc_state_t proc_state;
	int i;

	struct proc_stats *child;

	push_layer();
	now = time(NULL);

	write_stuff(&now, sizeof(now));


	sigar_proc_list_get(sigar, &proc_list);

	for (i = 0; i < proc_list.number; ++i) {
		sigar_pid_t pid = proc_list.data[i];

		if (sigar_proc_state_get(sigar, pid, &proc_state) != SIGAR_OK
		    || sigar_proc_cpu_get(sigar, pid, &proc_cpu) != SIGAR_OK
		    || sigar_proc_mem_get(sigar, pid, &proc_mem) != SIGAR_OK) {
			continue;
		}

		push_layer();

		write_stuff(&proc_state, sizeof(proc_state));
		write_stuff(&proc_cpu, sizeof(proc_cpu));
		write_stuff(&proc_mem, sizeof(proc_mem));

		pop_layer();
	}


	sigar_proc_list_destroy(sigar, &proc_list);

	pop_layer();
}

int main(void)
{
#ifdef _WIN32
    _setmode(1, _O_BINARY);
    _setmode(0, _O_BINARY);
#endif

    sigar_open(&sigar);

    while (1) {
	    write_sample();
	    fflush(stdout);
	    sleep(1);
    }

    return 0;
}
