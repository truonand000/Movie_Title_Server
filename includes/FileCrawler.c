/*
 *  Created by Adrienne Slaughter
 *  CS 5007 Spring 2019
 *  Northeastern University, Seattle
 *
 *  This is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  It is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  See <http://www.gnu.org/licenses/>.
 * 
 *  Edited by: Andrew Truong
 *  Date: 4/1/2019
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#include "FileCrawler.h"
#include "DocIdMap.h"
#include "LinkedList.h"


void CrawlFilesToMap(const char *dir, DocIdMap map) {
  struct stat s;
  struct dirent **namelist;
  int n;

  // Get stat information on the current item.
  stat(dir, &s);

  // If it is a file, put it in the map. If not, recursively DFSearch it for more files.
  if (S_ISDIR(s.st_mode) == 0) {
    PutFileInMap((char*)dir, map);
    // Print statement that reports files that were successfully placed into the map.
    printf("added %s to DocIdMap\n", dir);
    return;
  } else {
    n = scandir(dir, &namelist, 0, alphasort);
    for (int i = 2; i < n; i++) {
      // Create and malloc new string to be used as a reference file/directory name.
      char *newDirPath = (char*)malloc(strlen(dir)+strlen(namelist[i]->d_name)+2);
      strcpy(newDirPath, dir);
      strcat(newDirPath, namelist[i]->d_name);
      stat(newDirPath, &s);
      // Check if the new path is a file first, if it is then add it.
      // You will have to free this malloc'd string later down the line
      if (S_ISDIR(s.st_mode) == 0) {
	PutFileInMap(newDirPath, map);
	printf("added %s to DocIdMap\n", dir);
      } else {
	// Else, recursively check the directory and free the malloc'd string (directory path) afterwards
        strcat(newDirPath, "/");
        CrawlFilesToMap(newDirPath, map);
        free(newDirPath);
      } 
    }
    while (n--) {
      free(namelist[n]);
    }
  }
  if (namelist != NULL) {
   free(namelist);
  }
 return;
}
