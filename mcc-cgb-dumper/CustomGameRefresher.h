#pragma once
#include "MultilevelPointer.h"
#include "PointerData.h"

// Defines a CustomGameRefresh class that can force the CustomGameBrowser to refresh
// using a call to the MCC function that performs it.
// Includes a way to automatically force a refresh every however-many seconds.

// Define the func signature of the games CallRefreshFunction that we will want to call
typedef uint64_t(*MCC_CallRefreshFunction)(uint64_t unknownPointer);


class CustomGameRefresher
{
private:

	unsigned int mAutoRefreshInterval = 11; // How many seconds between each auto refresh
	unsigned long mAutoRefreshEnabled = false; // Whether the auto-refresh thread will do a refresh
	bool mNeedToDie = false; // set to true in destructor so autoRefreshThreadTask will end
	std::thread mAutoRefreshThread; // Will set this up in constructor after possible throws
	void autoRefreshThreadTask() const
	{
		while (!this->mNeedToDie)
		{
			if (this->mAutoRefreshEnabled)
			{
				this->forceRefresh();
				Sleep(this->mAutoRefreshInterval * 1000); // Convert from seconds to milliseconds
			}
			else
			{
				Sleep(100);
			}
		}
		PLOG_DEBUG << "CustomGameRefresher->mAutoRefreshThread concluded";
	}

	safetyhook::MidHook getParameterHook;
	static void getParameterHookFunction(safetyhook::Context64& ctx)
	{
		if (!parameterToPass)
		{
			parameterToPass = ctx.rcx;
			PLOG_DEBUG << "parameter captured: " << parameterToPass;
		}

	}



	static uint64_t parameterToPass;

	MCC_CallRefreshFunction mOrigCallRefreshFunction; // We'll resolve this in constructor

public:
	void forceRefresh() const // Calls the games CallRefreshFunction
	{
		if (!parameterToPass)
		{
			PLOG_ERROR << "Cannot force refresh until refresh has been manually called at least once";
			return;
		}


		PLOG_VERBOSE << "Performing CallRefreshFunction: parameter: " << std::hex << parameterToPass;
		mOrigCallRefreshFunction(parameterToPass);
		PLOG_VERBOSE << "CallRefreshFunction didn't crash!";

	}

	
	// Constructor - Resolves CallRefreshFunction and the pointer to the parameter it needs
	CustomGameRefresher()
	{
		parameterToPass = 0;
		void* pCallRefresh;
		if (!mlp_OrigCallRefreshFunction.resolve(&pCallRefresh))
		{
			throw std::runtime_error(std::format("Couldn't resolve address of CallRefreshFunction : {}", MultilevelPointer::GetLastError()));
		}
		mOrigCallRefreshFunction = (MCC_CallRefreshFunction)pCallRefresh;

		// Set up hook to capture the parameter that needs to be passed to the function by hooking the function
		auto builder = SafetyHookFactory::acquire();
		getParameterHook = builder.create_mid(pCallRefresh, getParameterHookFunction);

		mAutoRefreshThread = std::thread([this] { this->autoRefreshThreadTask(); }); // Thread is born when this class is constructed, only AFTER possible throws

	}


	// Destructor - needs to kill auto refresh thread
	~CustomGameRefresher()
	{
		this->mNeedToDie = true;
		if (this->mAutoRefreshThread.joinable())
		{
			// Join and wait for it to die
			this->mAutoRefreshThread.join();
		}
	}

	// For automatic refreshing on a timer, called by user via CommandAutoRefreshEnable
	void enableAutoRefresh(unsigned long intervalInSeconds)
	{
		// Tell the thread to force refreshes
		this->mAutoRefreshInterval = intervalInSeconds;
		this->mAutoRefreshEnabled = true;
	}

	// Disable above, called by user via CommandAutoRefreshDisable
	void disableAutoRefresh()
	{
		this->mAutoRefreshEnabled = false;
	}

};

