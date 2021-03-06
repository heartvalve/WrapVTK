/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParseMerge.c

  Copyright (c) 2010 David Gobbi
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  Please see Copyright.txt for more details.

=========================================================================*/

#include "vtkParseMerge.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* add a class to the MergeInfo */
unsigned long vtkParseMerge_PushClass(MergeInfo *info, const char *classname)
{
  unsigned long n = info->NumberOfClasses;
  unsigned long m = 0;
  unsigned long i;
  const char **classnames;
  char *cp;

  /* if class is already there, return its index */
  for (i = 0; i < n; i++)
    {
    if (strcmp(info->ClassNames[i], classname) == 0)
      {
      return i;
      }
    }

  /* if no elements yet, reserve four slots */
  if (n == 0)
    {
    m = 4;
    }
  /* else double the slots whenever size is a power of two */
  if (n >= 4 && (n & (n-1)) == 0)
    {
    m = (n << 1);
    }

  if (m)
    {
    classnames = (const char **)malloc(m*sizeof(const char *));
    if (n)
      {
      for (i = 0; i < n; i++)
        {
        classnames[i] = info->ClassNames[i];
        }
      free((char **)info->ClassNames);
      }
    info->ClassNames = classnames;
    }

  info->NumberOfClasses = n+1;
  cp = (char *)malloc(strlen(classname)+1);
  strcpy(cp, classname);
  info->ClassNames[n] = cp;

  return n;
}

/* add a function to the MergeInfo */
unsigned long vtkParseMerge_PushFunction(MergeInfo *info, unsigned long depth)
{
  unsigned long n = info->NumberOfFunctions;
  unsigned long m = 0;
  unsigned long i;
  unsigned long *overrides;
  unsigned long **classes;

  /* if no elements yet, reserve four slots */
  if (n == 0)
    {
    m = 4;
    }
  /* else double the slots whenever size is a power of two */
  else if (n >= 4 && (n & (n-1)) == 0)
    {
    m = (n << 1);
    }

  if (m)
    {
    overrides = (unsigned long *)malloc(m*sizeof(unsigned long));
    classes = (unsigned long **)malloc(m*sizeof(unsigned long *));
    if (n)
      {
      for (i = 0; i < n; i++)
        {
        overrides[i] = info->NumberOfOverrides[i];
        classes[i] = info->OverrideClasses[i];
        }
      free(info->NumberOfOverrides);
      free(info->OverrideClasses);
      }
    info->NumberOfOverrides = overrides;
    info->OverrideClasses = classes;
    }

  info->NumberOfFunctions = n+1;
  info->NumberOfOverrides[n] = 1;
  info->OverrideClasses[n] = (unsigned long *)malloc(sizeof(unsigned long));
  info->OverrideClasses[n][0] = depth;

  return n;
}

/* add an override to to the specified function */
unsigned long vtkParseMerge_PushOverride(
  MergeInfo *info, unsigned long i, unsigned long depth)
{
  unsigned long n = info->NumberOfOverrides[i];
  unsigned long m = 0;
  unsigned long j;
  unsigned long *classes;

  /* Make sure it hasn't already been pushed */
  for (j = 0; j < info->NumberOfOverrides[i]; j++)
    {
    if (info->OverrideClasses[i][j] == depth)
      {
      return i;
      }
    }

  /* if n is a power of two */
  if ((n & (n-1)) == 0)
    {
    m = (n << 1);
    classes = (unsigned long *)malloc(m*sizeof(unsigned long));
    for (j = 0; j < n; j++)
      {
      classes[j] = info->OverrideClasses[i][j];
      }
    free(info->OverrideClasses[i]);
    info->OverrideClasses[i] = classes;
    }

  info->NumberOfOverrides[i] = n+1;
  info->OverrideClasses[i][n] = depth;

  return n;
}

/* return an initialized MergeInfo */
MergeInfo *vtkParseMerge_CreateMergeInfo(ClassInfo *classInfo)
{
  unsigned long i, n;
  MergeInfo *info = (MergeInfo *)malloc(sizeof(MergeInfo));
  info->NumberOfClasses = 0;
  info->NumberOfFunctions = 0;

  vtkParseMerge_PushClass(info, classInfo->Name);
  n = classInfo->NumberOfFunctions;
  for (i = 0; i < n; i++)
    {
    vtkParseMerge_PushFunction(info, 0);
    }

  return info;
}

/* free the MergeInfo */
void vtkParseMerge_FreeMergeInfo(MergeInfo *info)
{
  unsigned long i, n;

  n = info->NumberOfClasses;
  for (i = 0; i < n; i++)
    {
    free((char *)info->ClassNames[i]);
    }
  free((char **)info->ClassNames);

  n = info->NumberOfFunctions;
  for (i = 0; i < n; i++)
    {
    free(info->OverrideClasses[i]);
    }
  if (n)
    {
    free(info->NumberOfOverrides);
    free(info->OverrideClasses);
    }

  free(info);
}

/* merge a function */
static void merge_function(FunctionInfo *merge, const FunctionInfo *func)
{
  if (func->IsVirtual)
    {
    merge->IsVirtual = 1;
    }

  if (func->Comment && !merge->Comment)
    {
    merge->Comment = func->Comment;
    }
}

/* add "super" methods to the merge */
unsigned long vtkParseMerge_Merge(
  MergeInfo *info, ClassInfo *merge, ClassInfo *super)
{
  unsigned long i, j, k, ii, n, m, depth;
  int match;
  FunctionInfo *func;
  FunctionInfo *f1;
  FunctionInfo *f2;

  depth = vtkParseMerge_PushClass(info, super->Name);

  m = merge->NumberOfFunctions;
  n = super->NumberOfFunctions;
  for (i = 0; i < n; i++)
    {
    func = super->Functions[i];

    if (!func || !func->Name)
      {
      continue;
      }

    /* constructors and destructors are not inherited */
    if ((strcmp(func->Name, super->Name) == 0) ||
        (func->Name[0] == '~' &&
         strcmp(&func->Name[1], super->Name) == 0))
      {
      continue;
      }

    /* check for overridden functions */
    match = 0;
    for (j = 0; j < m; j++)
      {
      f2 = merge->Functions[j];
      if (f2->Name && strcmp(f2->Name, func->Name) == 0)
        {
        match = 1;
        break;
        }
      }

    /* find all superclass methods with this name */
    for (ii = i; ii < n; ii++)
      {
      f1 = super->Functions[ii];
      if (f1 && f1->Name && strcmp(f1->Name, func->Name) == 0)
        {
        if (match)
          {
          /* look for override of this signature */
          for (j = 0; j < m; j++)
            {
            f2 = merge->Functions[j];
            if (f2->Name && strcmp(f2->Name, f1->Name) == 0)
              {
              if (f2->NumberOfParameters == func->NumberOfParameters)
                {
                for (k = 0; k < f2->NumberOfParameters; k++)
                  {
                  if (f2->Parameters[k]->Type != func->Parameters[k]->Type &&
                      strcmp(f2->Parameters[k]->TypeName,
                             func->Parameters[k]->TypeName) == 0)
                    {
                    break;
                    }
                  }
                /* if all args match, then merge the comments */
                if (k == f2->NumberOfParameters)
                  {
                  merge_function(f2, func);
                  vtkParseMerge_PushOverride(info, j, depth);
                  }
                }
              }
            }
          }
        else /* no match */
          {
          /* copy into the merge */
          vtkParse_AddFunctionToClass(merge, f1);
          vtkParseMerge_PushFunction(info, depth);
          m++;
          }
        /* remove from future consideration */
        super->Functions[ii] = NULL;
        }
      }
    }

  /* remove all used methods from the superclass */
  j = 0;
  for (i = 0; i < n; i++)
    {
    if (i != j && super->Functions[i] != NULL)
      {
      super->Functions[j++] = super->Functions[i];
      }
    }
  super->NumberOfFunctions = j;

  return depth;
}
