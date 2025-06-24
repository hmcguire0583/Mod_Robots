# Mod_Robots
Implementation of the locate and free algorithm by DFS -> Harry, Russ, and Fouad

TO DO:
1. Take code made and make new LocateAndFree class
    - Make a friend to Lattice
2. Create function to find the start S (Greatest Z, then smallest X & Y)
3. Find a way to print path of DFS (starting from S)
4. After the end of DFS move block one direction Towards smallest
5. Create a configuration in 3d to test.
   
# Commands

mkdir ./cmake-free
cmake -S . -B cmake-free -DCMAKE_TOOLCHAIN_FILE="$PWD/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build ./cmake-free --target LocateAndFree
cmake --build ./cmake-free --target ColorPropertyLib
cmake --build ./cmake-free --target OrientationPropertyLib

# Sample test run
.\cmake-free\LocateAndFree.exe -I C:\Users\Owner\Mod_Robots\testing-robots\initial.json -F C:\Users\Owner\Mod_Robots\testing-robots\final.json -e harry
