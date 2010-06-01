/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParseHierarchy.h

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  Please see Copyright.txt for more details.

=========================================================================*/

/*
 This file contains utility functions for loading and parsing
 a VTK hierarchy file.  The file contains entries like the
 following (one per line in the file): 

 classname [ : superclass ] ; header.h
*/

#ifndef VTK_PARSE_HIERARCHY_H
#define VTK_PARSE_HIERARCHY_H

typedef struct _HierarchyEntry
{
  char *ClassName;
  char *HeaderFile;
  char *SuperClasses[10];
  int SuperClassIndex[10];
} HierarchyEntry;

typedef struct _HierarchyInfo
{
  int NumberOfClasses;
  HierarchyEntry *Classes;
} HierarchyInfo;

#ifdef __cplusplus
extern "C" {
#endif

/* read a hierarchy file into a HeirarchyInfo struct, or return NULL */
HierarchyInfo *vtkParseHierarchy_ReadFile(const char *filename);

/* free a HierarchyInfo struct */
void vtkParseHierarchy_Free(HierarchyInfo *info);

/* check whether class 1 is a subclass of class 2 */
int vtkParseHierarchy_IsTypeOf(
  HierarchyInfo *info, const char *subclass, const char *superclass);

/* get the header file for the specified class */
const char *vtkParseHierarchy_ClassHeader(
  HierarchyInfo *info, const char *classname);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
