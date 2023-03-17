#pragma once

// Defines a CustomGameRefresh class that can simulate a click in the MCC process
// at the position where the Custom Game Browser's refresh button should be.
// Includes a function to set the position of the refresh button,
// and a way to automatically click refresh every however-many seconds.




class CustomGameRefresher
{
private:
	// The position of the center of the refresh button, so we can send simulated clicks at its position
	WORD mRefreshX = 0;
	WORD mRefreshY = 0;
	bool mRefreshPositionInitialized = false; // false until setRefreshClickPosition or guessRefreshClickPosition ran

	unsigned int mAutoRefreshInterval = 11; // How many seconds between each auto refresh
	unsigned long mAutoRefreshEnabled = false; // Whether the auto-refresh thread will do a refresh
	bool mNeedToDie = false; // set to true in destructor so autoRefreshThreadTask will end
	std::thread mAutoRefreshThread = std::thread([this] { this->autoRefreshThreadTask(); }); // Thread is born when this class is constructed
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


public:
	void forceRefresh() const; // Sends a simulated mouse click at the refresh button position
	void tryGuessRefreshClickPosition(); // Gets the MCC window dimensions and guesses the refresh button position
	void moveCursorToRefreshClickPosition(); // Moves the cursor to the currently set position so user can see if it lines up with the actual refresh button

	// Called by user via CommandSetRefreshClickPosition
	void setRefreshClickPosition(WORD x, WORD y)
	{
		mRefreshX = x;
		mRefreshY = y;
		mRefreshPositionInitialized = true;

		moveCursorToRefreshClickPosition();
	}

	// Constructor - tries to guess the refresh button position
	CustomGameRefresher()
	{
		// this function may fail, which is fine - mRefreshPositionInitialized will be left
		// as false, and forceRefresh will check for that
		tryGuessRefreshClickPosition(); 
		moveCursorToRefreshClickPosition();
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

