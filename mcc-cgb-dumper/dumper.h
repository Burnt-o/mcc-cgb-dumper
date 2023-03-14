#pragma once
#include "global_kill.h"
namespace dumper
{
	void dump();


	class AutoDumper {
	private:
		static AutoDumper& get() {
			static AutoDumper instance;
			return instance;
		}

		AutoDumper() = default;
		~AutoDumper() = default;



		DWORD mInterval = 11;
		bool mEnabled = false;

		std::thread ADthread = std::thread([]() {
			while (!global_kill::is_kill_set())
			{
				if (AutoDumper::get().mEnabled)
				{
					dump();
					Sleep(AutoDumper::get().mInterval * 1000);
				}
				else
				{
					Sleep(100);
				}

			}
			return;
			});

	public:
		

		static void Enable(DWORD intervalInSeconds)
		{
			AutoDumper::get().mInterval = intervalInSeconds;
			AutoDumper::get().mEnabled = true;
		}

		static void Disable()
		{
			AutoDumper::get().mEnabled = false;
		}

		static std::thread& GetThreadRef()
		{
			return AutoDumper::get().ADthread;
		}


	};

};

