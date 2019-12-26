#include "quickjs_worker.h"
#include "core/os/os.h"

void QuickJSWorker::thread_main(void *p_this) {
	QuickJSWorker *self = static_cast<QuickJSWorker*>(p_this);

	self->initialize();
	self->running = true;

	Error err;
	String text = FileAccess::get_file_as_string(self->entry_script, &err);

	if (err == OK) {
		String err_text;
		err = self->safe_eval_text(text, self->entry_script, err_text);
		if (err == OK) {
			while (self->running) {
				self->frame();
				OS::get_singleton()->delay_usec(1000);
			}
		} else {
			ERR_PRINTS("Failed to eval entry script:" + self->entry_script + "\nError:" + err_text);
		}
	} else {
		ERR_PRINTS("Failed to load entry script:" + self->entry_script);
	}

	self->uninitialize();
}

QuickJSWorker::QuickJSWorker(): QuickJSBinder() {
	thread = NULL;
	running = false;
}

QuickJSWorker::~QuickJSWorker() {
	stop();
}

void QuickJSWorker::initialize() {
	QuickJSBinder::initialize();
}

void QuickJSWorker::uninitialize() {
	QuickJSBinder::uninitialize();
}

void QuickJSWorker::start(const String &p_path) {
	ERR_FAIL_COND(running || thread != NULL);
	entry_script = p_path;
	thread = Thread::create(thread_main, this);
}

void QuickJSWorker::stop() {
	if (thread != NULL) {
		if (running) {
			running = false;
			Thread::wait_to_finish(thread);
		}
		memdelete(thread);
		thread = NULL;
	}
}
