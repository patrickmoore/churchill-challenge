#Introduction
This is my submitted solution for the Churchill Navigation Programming Challenge  http://churchillnavigation.com/challenge/.  Unfortunately my solution was fairly slow compared to the competition, but the algorithms do have some qualities to them.

The problem is:  given 10M random points, find the top 20 ranked points in an area defined by a random rectangle.

I ended up with several different solutions in trying to climb the top of the standings.
* **Linear Search** - used for testing and comparison of data to the more complicated algorithms
* **KdTree** - morphed into a binary r-tree then abandoned when results were not good enough.
* **Boost RTree** - Implemented to get an idea how fast an R* Tree can be.
* **R* Tree** - The algorithm that was submitted for the contest.
* **HashGrid** - Started on the last day of the contest.  Showed promise, but I didn't get it fast enough in time.


#The Solution

#R* Tree
##Building
The algorithm for building the R* Tree was quite complicated.  Significant effort was made in order to reduce minimum bound box (mbr) overlap and partition the data so that nodes were as balanced as possible.  

I initially was looking at Sort-Tile-Recursive for bulk loading, but I ended up creating a Top-Down bulk loading algorithm based on the algorithm used to construct the KdTree.  This allowed for no overlap of mbrs for the nodes which means less traversals in a search.  I added the ability to define the min and max value of nodes and a max value for the number of elements in the leaf.  This allowed me to have some control over the cache locality.

The nodes and elements in the leafs were then sorted by rank to provide early break out during a search.

##Searching
There are both recursive and iterative search algorithms.  The iterative search ended up being significantly faster and thus was used.  Each node in a level is tested with the requested rect and then added to the task stack if there is an intersection.  Since the nodes are ordered by rank, an early breakout can occur if the node being checked is larger than the current result set.  The same happens for the elements of a leaf.


#Dataset Partition
In order to reduce the searchable dataset, the dataset was order by rank and partitioned.  Each partition was then inserted in to its respective R* Tree.  If the result set was full at the end of checking a partition, the rest of the partitions are skipped and the currently result set is returned.

#Statistic Analysis
Investigating worst case scenarios, it was found that small or thin search rects were the worst performers.  In these cases, a linear search of the set was superior, but only when there was a relatively small number of points (there are several cases where many points are grouped tightly together).  Predicting the number of points in a region allowed me to choose between using a linear search or the tree search.

The probability of a point being within the query rect is given by the Standard Normal Distribution function

![](http://upload.wikimedia.org/math/9/8/a/98a550e5fc94e32d527528ccb45da54b.png)

where σ is the standard deviation and μ is the mean.

Predicting the number of points in a region happens as follows:
* During building, calculate the mean and standard deviation for the dataset.
* During a search, calcuate the probability of the min and then the max edge of the search region by the Standard Normal Distribution function.
* Subtract the max and min probability
* Multiply the resulting probability with the number of points in the dataset.

Though not exact (since there isn't a guarentee that this is a standard normal distribution), the analysis gave good results that through some tweaking allowed for confident decisions to be made based off of the distribution of the dataset and search region.

#Linear Search
Based on the statistical analysis, if the predicted dataset within the search region is relatively small, a simple linear search of data sorted by the smallest predicted edge is very fast.  This required duplicating the dataset for both the x and y dimensions, but there was plenty of memory for that.

#Miscellanous Improvements
###The Result Set
I spent some time optimizing the recording of the results into the set as that was at one point a major hotspot when profiling. The solution I came up with was that during the search, to only sort the largest element when the result set was full.  Comparisons for early breakout of the nodes/elements can then happen by only comparing the last element in the set.  When a new element was added, it replaced the largest at the end and then swapped with the next largest element.  The final result set was then sorted once at the end before being reported.

###Compile Time Polymorphism
I accomplished this by using CRTP. This wasn't needed for speed of the submission of the solution, but it allowed fast comparisons of differnet algorithms and reduced iteration time without the cost of virtual table calls.


#Lessons
* No difference between aligned and unaligned Point structs.
* The cost of using multithreading (thread startup, syncronization, etc) was about the same as the time it took to perform the query, negating any benefits.  Multithreading did help in building the by using parallel_sort from the Microsoft PPL library.
* The fastest solutions used intrinsics to perform fast comparisons of multiple floats at the same time.  I should known that their use would be crucial for fast times.

#Improvements
Intrinsics.  In both R* Tree nodes and elements.

I started work on HashGrid at the end of the contest as a way to resolve the issue of cache misses hurting the R* Tree solution, but I was not able to finish.  My idea was to have a sorted datasets in both x and y dimensions with the HashGrid containing indexes to both.  This solution shows promise especially with the usage of AVX intrinsics.

