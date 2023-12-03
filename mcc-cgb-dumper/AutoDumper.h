#pragma once
#include "GlobalKill.h"
#include "MultilevelPointer.h"
#include "InitParameter.h"
#include "PointerData.h"
	// Hooks the MCC function that populates the CustomGameServer info so we know when to dump it all to json
	class AutoDumper {
	private:
		typedef char(*MCC_UpdateCustomGameArray)(uint64_t unknown);

		void dump();

		static bool mDumpQueued; // Set to true when a dump wants to happen
		bool mNeedToDie = false; // set to true in destructor so autoRefreshThreadTask will end
		std::thread mDumpQueueThread; // Will set this up in constructor after possible throws

		// Working thread that does the dumping, dumps are queued by hkUpdateCustomGameArray
		void DumpQueueThreadTask()
		{

			while (!this->mNeedToDie)
			{
				if (AutoDumper::mDumpQueued)
				{
					PLOG_DEBUG << "DumpQueueThreadTask saw a dump was queued! performing dump";
					dump();
					AutoDumper::mDumpQueued = false;
				}
				Sleep(100);
			}
		}

		static MCC_UpdateCustomGameArray mOrigUpdateCustomGameArray; // The Original UpdateCustomGameArray, will resolve this in constructer and set up hook there so it's redirected to hkUpdateCustomGameArray
		
		// Our hook, we'll let the games UpdateCustomGameArray run then queue a dump to happen in our thread
		static char hkUpdateCustomGameArray(uint64_t unknown)
		{
			PLOG_VERBOSE << "hkUpdateCustomGameArray running!";
			char returnVal = mUpdateCustomGameArrayHook.call<char, uint64_t>(unknown);
			PLOG_VERBOSE << "calling mOrigUpdateCustomGameArray didn't crash!";
			AutoDumper::QueueDump();
			return returnVal;
		}

		// hook object
		static safetyhook::InlineHook mUpdateCustomGameArrayHook;

		std::string mJsonDumpPath;

	public:
		
		// Constructor - attaches hook and sets up thread
		AutoDumper()
		{
			mJsonDumpPath = std::string(g_ourInitParameters->injectorPath);


			void* pOrigUpdateCustomGameArray;
			if (!mlp_OrigUpdateCustomGameArray.resolve(&pOrigUpdateCustomGameArray))
			{
				throw std::runtime_error(std::format("Couldn't resolve address of mlp_OrigUpdateCustomGameArray : {}", MultilevelPointer::GetLastError()));
			}
			AutoDumper::mOrigUpdateCustomGameArray = (MCC_UpdateCustomGameArray)pOrigUpdateCustomGameArray;

			// attach hook
			auto builder = SafetyHookFactory::acquire();
			PLOG_DEBUG << "Hooking UpdateCustomGameArray at: " << mOrigUpdateCustomGameArray;
			PLOG_DEBUG << "to: " << &AutoDumper::hkUpdateCustomGameArray;
			AutoDumper::mUpdateCustomGameArrayHook = builder.create_inline(mOrigUpdateCustomGameArray, &AutoDumper::hkUpdateCustomGameArray);

			this->mDumpQueueThread = std::thread([this] { this->DumpQueueThreadTask(); }); // Thread is born when this class is constructed, only AFTER possible throws
		}

		// Destructor - unattaches hook and kills thread
		~AutoDumper()
		{
			AutoDumper::mUpdateCustomGameArrayHook.reset(); // unhook

			// set kill flag then join and wait for it to die
			this->mNeedToDie = true;
			if (this->mDumpQueueThread.joinable())
			{
				// Join and wait for it to die
				this->mDumpQueueThread.join();
			}
		}

		bool setJsonDumpPath(std::string_view const &path)
		{
			// Validate input (path is a valid directory)
			PLOG_DEBUG << "setJsonDumpPath passed following path: " << path;
			std::string testPath(path.data());
			testPath.append("//CustomGameBrowserData.json");
			std::ofstream outFile(testPath);
			if (outFile.is_open())
			{
				outFile.close();
				PLOG_DEBUG << "path is valid";
				mJsonDumpPath.assign(path);
				return true;
			}
			else
			{
				PLOG_ERROR << "Failed to open path : " << GetLastError();
				return false;
			}
		}

		const std::string_view getJsonDumpPath()
		{
			return mJsonDumpPath;
		}


		static void QueueDump()
		{
			PLOG_DEBUG << "Dump queued";
			mDumpQueued = true;
		}
	};

