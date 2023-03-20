#pragma once
#include "multilevel_pointer.h"

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


	//const multilevel_pointer mlp_OrigCallRefreshFunction{ { 0xA92EA8 } }; // Pointer to OrigCallRefresh
	const multilevel_pointer mlp_OrigCallRefreshFunction{ { 0xA785A4 } }; // Pointer to OrigCallRefresh
	//const multilevel_pointer mlp_OrigCallRefreshParameter{ { 0x03B227C8, 0x90, 0xB0 } }; // Parameter that needs to be passed to OrigCallRefresh
	//const multilevel_pointer mlp_OrigCallRefreshParameter{ { 0x0401C130, 0x18, 0x830, 0x820 } }; // Parameter that needs to be passed to OrigCallRefresh
	const multilevel_pointer mlp_OrigCallRefreshParameter{ { 0x03D0B6A0, 0x28, 0xE90, 0x0 } }; // Parameter that needs to be passed to OrigCallRefresh
	MCC_CallRefreshFunction mOrigCallRefreshFunction; // We'll resolve this in constructor

public:
	void forceRefresh() const // Calls the games CallRefreshFunction
	{
		void* parameter;
		if (!mlp_OrigCallRefreshParameter.resolve(&parameter))
		{
			PLOG_ERROR << "forceRefresh failed, couldn't read parameter";
			return;
		}



		PLOG_VERBOSE << "Performing CallRefreshFunction: parameter: " << std::hex << parameter;
		mOrigCallRefreshFunction((uint64_t)parameter);
		PLOG_VERBOSE << "CallRefreshFunction didn't crash!";

	}

	
	// Constructor - Resolves CallRefreshFunction and the pointer to the parameter it needs
	CustomGameRefresher()
	{
		
		void* pCallRefresh;
		if (!mlp_OrigCallRefreshFunction.resolve(&pCallRefresh))
		{
			throw std::runtime_error(std::format("Couldn't resolve address of CallRefreshFunction : {}", multilevel_pointer::GetLastError()));
		}
		mOrigCallRefreshFunction = (MCC_CallRefreshFunction)pCallRefresh;


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

