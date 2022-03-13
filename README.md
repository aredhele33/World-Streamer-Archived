# World Streamer (Archived)
#### This is an archived project. Since this project is maintained, sources are provided.

<p align="center">
  <img src="https://raw.githubusercontent.com/Aredhele/World-Streamer-Archived/main/Press/OpenWorldStreamer_Overview.png" />
</p>

<p align="center">
  <b>Important : This is project will only run on Windows 10 and later since the code relies on the Win32 API. 
  This project provides a ready-to-use Visual Studio 2022 solution, see the tutorial part for more information.</b>
</p>

## Abstract
World Streamer (Archived) is a 2 days project to experiment **asynchronous I/O and streaming**. I wanted to try it out
with a small and fun example so I created a small program where you can move around in a big procedural world. The world is 
not entirely loaded in memory at once, only the near area around the player is loaded.

### What is World Streamer (Archived)

* A very small project (less than 10 files) and minimalistic (straight forward code).
* A fake open world simulation, but a stable simulation, should not crash

### What is NOT World Streamer (Archived)

* Robust code
* Professional code
* Fun, you can only move around

External dependencies :

| Name                                          | Used for             |
|-----------------------------------------------|----------------------|
| [sfml](https://github.com/SFML/SFML)          | Multimedia utilities |
| [zlib & gzip](https://github.com/madler/zlib) | Compression utility  |

## How to install ?

1) Clone or download the repository :
```bash 
git clone https://github.com/Aredhele/World-Streamer-Archived
```
2) Then go into the **Projects** folder in World-Streamer-Archived/Projects and open **WorldStreamer.sln**
``` 
The solution is normally portable and should work on your computer without any extra change.
```
3) It should open the solution, select Release or Debug (as you want) and hit CTRL+SHIFT+B or the build button
4) If you have selected Release, you should now have the following Bin folder :
<p align="center">
  <img src="https://raw.githubusercontent.com/Aredhele/World-Streamer-Archived/main/Press/OpenWorldStreamer_BinaryFolder.png" />
</p>

## How to run ?
We need to generate a random world to be able to run it. Note that you already have a program named **WorldDataGenerator_r.exe**.
``` 
WorldDataGenerator_r.exe is located in Bin/Release/. There's no debug version of this program.
```
1) Open a command prompt
   1) On Windows 10, you can type "cmd" in the window path field to open the console in the current folder
   2) On Windows 11, just right click and select "Open in Windows Terminal"
2) Then generate a world using **WorldDataGenerator_r.exe** with the following command line :
```bash 
WorldDataGenerator_r.exe 256 256 64 game_data.bin
```
3) It will take seconds to proceed, and you have the following
<p align="center">
  <img src="https://raw.githubusercontent.com/Aredhele/World-Streamer-Archived/main/Press/OpenWorldStreamer_GenerationOutput.png" />
</p>

``` 
The generator is not robust and the quality of the generation depends on the grid size. I strongly recommend to use a grid 256x256.
Other values will work but the environment may lack of trees or water etc. 
```
4) Now you can run the game with the following command :
```bash 
WorldStreamer_r.exe 0 0 game_data.bin
```
<p align="center">
  <img src="https://raw.githubusercontent.com/Aredhele/World-Streamer-Archived/main/Press/OpenWorldStreamer_Window.png" />
</p>

## How to use it ?

To understand the behavior of the program, here are some basic information :
<p align="center">
  <img src="https://raw.githubusercontent.com/Aredhele/World-Streamer-Archived/main/Press/OpenWorldStreamer_WindowInfo.png" />
</p>

If you zoom out a bit :
<p align="center">
  <img src="https://raw.githubusercontent.com/Aredhele/World-Streamer-Archived/main/Press/OpenWorldStreamer_WindowInfo2.png" />
</p>

Input

| Action                             | Input                        |
|------------------------------------|------------------------------|
| Move up, right, down, left         | Arrow keys                   |
| Speed boost                        | Hold left shift while moving |
| Zoom in / out                      | num pad + / - (or A / E)     |
| Increase / decrease main grid size | num pad / / * (or F / G)     |

Output examples (Zooming out, moving slowly and loading cells) :

Zoomming out                                                                                                      | Slow move (human speed)
:----------------------------------------------------------------------------------------------------------------:|:--------------------------------------------------------------------------------------------------------------:
![](https://raw.githubusercontent.com/Aredhele/World-Streamer-Archived/main/Press/OpenWorldStreamer_ZoomOut.gif)  |  ![](https://raw.githubusercontent.com/Aredhele/World-Streamer-Archived/main/Press/OpenWorldStreamer_Slow.gif)
**Loading cells**                                                                                                 | **Fast move (debug speed)**
![](https://raw.githubusercontent.com/Aredhele/World-Streamer-Archived/main/Press/OpenWorldStreamer_Loading.gif)  |  ![](https://raw.githubusercontent.com/Aredhele/World-Streamer-Archived/main/Press/OpenWorldStreamer_Fast.gif)

## What does the binary game file contain ?

Here's an quick overview of the binary game file layout (can be reverse engineered from the engine source code) :

<p align="center">
  <img src="https://raw.githubusercontent.com/Aredhele/World-Streamer-Archived/main/Press/OpenWorldStreamer_Layout.png" />
</p>

Note that the cell buffers are compressed. It saves a lot of disk space for a near 0 cost at runtime (since buffers are small).
From my tests, a high compression rate can reduce the size of the file up to x10.

## How does the streamer work ?

The streamer has to open the game file with a special flag **FILE_FLAG_OVERLAPPED**. 
It means that we will be able to perform asynchronous non-blocking I/O on this file with a special read function.
Most of the time, when you read a file, this a blocking operation, and you have to wait until the function returns.
In my case, I won't create a new thread for each read request, and I would like to possibly be able to cancel the read request.

```cpp
// Reopening for async I/O
m_gameDataFile = CreateFile(
     Engine::GetInstance()->GetGameDataFileName(),
     GENERIC_READ,
     0,
     NULL,
     OPEN_EXISTING,
     FILE_FLAG_RANDOM_ACCESS | FILE_ATTRIBUTE_READONLY | FILE_FLAG_OVERLAPPED,
     NULL);
```
Then, the grid manager will populate a collection of cell requests and the streamer will pull requests from this collection.
If there're cells to load that are not already loaded, or being loaded, the streamer will initiate an asynchronous read :

```cpp
LARGE_INTEGER offsetLarge {};

// We know where is the cell buffer in the game file thanks to the precomputed offset
offsetLarge.QuadPart               = m_cellTable[m_requestToProcess[i].cellID].offset; 
result->asynchIORequest.Offset     = offsetLarge.LowPart;
result->asynchIORequest.OffsetHigh = offsetLarge.HighPart;
result->asynchIORequest.hEvent     = CreateEvent(NULL, TRUE, FALSE, NULL);

result->cellID    = m_requestToProcess[i].cellID;
result->allocInfo = bufferAlloc.GetFreeBuffer();

// Ask Windows to perform the read without blocking the thread
if (ReadFileEx(m_gameDataFile,
    result->allocInfo->bytes.data(), 
    static_cast<DWORD>(m_cellTable[m_requestToProcess[i].cellID].size), &result->asynchIORequest, FileIOCompletionRoutine) == 0)
{
   // Error management
}
```

Once the data is read, Windows will notify us through a user defined callback (FileIOCompletionRoutine) :

```cpp 
VOID CALLBACK FileIOCompletionRoutine(
	_In_     DWORD dwErrorCode,
	_In_     DWORD dwNumberOfBytesTransfered,
	_Inout_  LPOVERLAPPED lpOverlapped
)
{
   if (dwErrorCode != 0)
   {
      LOG_ERR("Async IO returns with a failure (", dwErrorCode);
   }
   else
   {
      // The read is complete! Let's notify the engine to deserialize the cell and add it to the world
      Streamer::GetInstance()->NotifyCellRequestCompleted(*lpOverlapped, dwNumberOfBytesTransfered);
   }
}
```

After having managed the cell request status in the streamer and the grid manager, the cell is ready to be deserialized.
It's basically the reverse operation of what the WorldDataGenerator_r.exe is doing (CF the game file layout).

```cpp 
void World::OnCellAddToWorld(const CellLoadingResult& cell)
{
   // This step is a kind of synchronization operation and must be under a lock (we're changing the world's state)
   m_mutex.lock();

   /// [...]
   
   // A cell is loaded. The following method will deserialize the cell and add it to the world.
   WorldCellData* worldCell = m_worldCells.emplace_back(new WorldCellData());
   worldCell->CellID = cell.cellID;

   // Step 0 : Cell buffer decompression
   // Uncompress buffer. Don't forget that cells are small compressed buffers to save disk space
   const std::string& decompressed_data = gzip::decompress(
      (char*)cell.allocInfo->bytes.data(), cell.allocInfo->bytes.size());

   // Step 1 : deserialization of the terrain and its LODs
   /// [...]

   // Step 2 : deserialization of the tiles
   /// [...]

   // Step 3 : deserialization of the environment
   /// [...], note that is as simple as using a basic memcpy to get the data, everything is already cooked and ready to use
   memcpy(&EnvTileSize, decompressed_data.data() + offset, sizeof(uint64_t));  offset += sizeof(uint64_t);
   
   m_mutex.unlock();
}
```

Nothing much! That's how an open world works if you remove all features and optimizations. 
Keep in mind that this example is in 2D. In a real 3D game, **it would be much more complicated.**

Thanks for reading!