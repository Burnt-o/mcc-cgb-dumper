#pragma once
#include "multilevel_pointer.h"

// Defines a CustomGameRefresh class that can simulate a click in the MCC process
// at the position where the Custom Game Browser's refresh button should be.
// Includes a function to set the position of the refresh button,
// and a way to automatically click refresh every however-many seconds.

// Define the games CallRefreshFunction
typedef uint64_t(*CallRefreshFunction)(uint64_t unknownPointer);


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


	const multilevel_pointer mlp_OrigCallRefreshFunction{ { 0xA92EA8 } }; // Pointer to OrigCallRefresh
	const multilevel_pointer mlp_OrigCallRefreshParameter{ { 0x03B227C8, 0x90, 0xB0 } }; // Parameter that needs to be passed to OrigCallRefresh
	CallRefreshFunction mOrigCallRefreshFunction; // We'll resolve this in constructor
	uint64_t* mpCallRefreshFunctionParameter = nullptr;

public:
	void forceRefresh() const // Calls the games CallRefreshFunction
	{
		if (IsBadReadPtr(mpCallRefreshFunctionParameter, sizeof(uint64_t)))
		{
			PLOG_ERROR << "forceRefresh failed, couldn't read parameter";
			return;
		}

		PLOG_VERBOSE << "Performing CallRefreshFunction:";
		mOrigCallRefreshFunction(*mpCallRefreshFunctionParameter);
		PLOG_VERBOSE << "CallRefreshFunction didn't crash!";
	}


	// Resolves CallRefreshFunction and its parameter
	CustomGameRefresher()
	{
		
		std::string errString;
		void* pCallRefresh;
		if (!mlp_OrigCallRefreshFunction.resolve(&pCallRefresh))
		{
			errString = "Couldn't resolve address of CallRefreshFunction : ";
			errString += multilevel_pointer::GetLastError();
			throw std::runtime_error(errString);
		}
		mOrigCallRefreshFunction = (CallRefreshFunction)pCallRefresh;

		void* pCallFunctionParameter; // The address *IS* the parameter
		if (!mlp_OrigCallRefreshParameter.resolve(&pCallFunctionParameter))
		{
			errString = "Couldn't resolve pointer to refresh function parameter : ";
			errString += multilevel_pointer::GetLastError();
			throw std::runtime_error(errString);
		}
		mpCallRefreshFunctionParameter = (uint64_t*)pCallFunctionParameter;

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

