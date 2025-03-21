#include <iostream>
#include <sstream>
#include <vector>
using namespace std;
  
// function to heapify the tree
void heapify(int arr[], int n, int root)
{
   // Initialize largest as root
   int largest = root;
   int left = 2*root + 1;
   int right = 2*root + 2;
   
   // If left child is larger than root
   if (left < n && arr[left] > arr[largest]){
       largest = left;
   }
   
   // If right child is larger than largest so far
   if (right < n && arr[right] > arr[largest]){
       largest = right;
   }
   
   // If largest is not root
   if (largest != root){
       swap(arr[root], arr[largest]);
       
       // Recursively heapify the affected sub-tree
       heapify(arr, n, largest);
   }
}
  
// implementing heap sort
void heapSort(int arr[], int n)
{
   // Build heap (rearrange array)
   for (int i = n/2 - 1; i >= 0; i--){
       heapify(arr, n, i);
   }
   
   // One by one extract an element from heap
   for (int i = n - 1; i > 0; i--){
       // Move current root to end
       swap(arr[0], arr[i]);
       
       // Call max heapify on the reduced heap
       heapify(arr, i, 0);
   }
}
  
/* print contents of array */
void displayArray(int arr[], int n)
{
   for (int i = 0; i < n; ++i)
       cout << arr[i] << " ";
   cout << "\n";
}
  
// main program
int main()
{
    string input;
    getline(cin, input); // Read input from the user

    stringstream ss(input);
    int num;
    vector<int> heap_arr;

    // Parse the input into an array
    while (ss >> num) {
        heap_arr.push_back(num);
    }

    int n = heap_arr.size();
  
    // Perform heap sort
    heapSort(heap_arr.data(), n);
   
    // Display