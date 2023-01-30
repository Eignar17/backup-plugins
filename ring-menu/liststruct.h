/*
 * Compiz ring/pie menu plugin
 *
 * liststruct.h
 *
 * Copyright : (C) 2010 by Matvey "blackhole89" Soloviev
 * E-mail    : blackhole89@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <string.h>

typedef void (*MultiListStructProc) (void *object, void *closure);

static void *
processMultiList (unsigned int        structSize,
		  void                *currData,
		  unsigned int        *numReturn,
		  MultiListStructProc init,
		  MultiListStructProc fini,
		  void                *closure,
		  unsigned int        numOptions,
		  ...)

{
    CompOption     *option;
    CompListValue  **options;
    unsigned int   *offsets;
    unsigned int   i, j, nElements = 0;
    unsigned int   oldSize;
    char           *rv, *value, *newVal, *setVal;
    va_list        ap;
    Bool           changed;

    CompOptionValue zeroVal, *optVal;

    char           **stringValue, **stringValue2;
    CompMatch      *matchValue;

    if (!numReturn)
	return NULL;
    oldSize = *numReturn;

    options = malloc (sizeof (CompOption *) * numOptions);
    if (!options)
	return currData;

    offsets = malloc (sizeof (unsigned int) * numOptions);
    if (!offsets)
    {
	free (options);
	return currData;
    }

    newVal = malloc (structSize);
    if (!newVal)
    {
	free (options);
	free (offsets);
	return currData;
    }

    va_start (ap, numOptions);

    for (i = 0; i < numOptions; i++)
    {
	option = va_arg (ap, CompOption *);
	offsets[i] = va_arg (ap, unsigned int);
	
	if (option->type != CompOptionTypeList)
	{
	    free (options);
	    free (offsets);
	    free (newVal);
	    va_end (ap);
	    return currData;
	}
	
	options[i] = &option->value.list;
	nElements = MAX (nElements, options[i]->nValue);
    }
    va_end (ap);

    for (j = nElements; j < oldSize; j++)
    {
	(*fini) (((char *)currData) + (j * structSize), closure);
	for (i = 0; i < numOptions; i++)
	{
	    value = ((char *)currData) + (j * structSize) + offsets[i];
	    switch (options[i]->type)
	    {
	    case CompOptionTypeString:
		stringValue = (char **) value;
		if (*stringValue)
		    free (*stringValue);
		break;
            case CompOptionTypeMatch:
		matchValue = (CompMatch *) value;
		matchFini (matchValue);
		break;
	    default:
		break;
	    }
	}
    }

    if (!nElements)
    {
	free (options);
	free (offsets);
	free (newVal);
	free (currData);
	*numReturn = 0;
	return NULL;
    }

    if (oldSize)
	rv = realloc (currData, nElements * structSize);
    else
        rv = malloc (nElements * structSize);

    if (!rv)
    {
	free (options);
	free (offsets);
	free (newVal);
	return currData;
    }

    if (nElements > oldSize)
	memset (rv + (oldSize * structSize), 0,
		(nElements - oldSize) * structSize);

    memset (&zeroVal, 0, sizeof (CompOptionValue));

    for (j = 0; j < nElements; j++)
    {
	changed = (j >= oldSize);
	memset (newVal, 0, structSize);
	for (i = 0; i < numOptions; i++)
	{
	    value = rv + (j * structSize) + offsets[i];
	    setVal = newVal + offsets[i];

	    if (j < options[i]->nValue)
		optVal = &options[i]->value[j];
	    else
		optVal = &zeroVal;

	    if (j < options[i]->nValue)
	    {
		switch (options[i]->type)
		{
		case CompOptionTypeBool:
		    memcpy (setVal, &optVal->b, sizeof (Bool));
		    changed |= memcmp (value, setVal, sizeof (Bool));
		    break;
		case CompOptionTypeInt:
		    memcpy (setVal, &optVal->i, sizeof (int));
		    changed |= memcmp (value, setVal, sizeof (int));
		    break;
		case CompOptionTypeFloat:
		    memcpy (setVal, &optVal->f, sizeof (float));
		    changed |= memcmp (value, setVal, sizeof (float));
		    break;
		case CompOptionTypeString:
		    stringValue = (char **) malloc(strlen(optVal->s) + 1);
		    strcpy(*stringValue, optVal->s);
		    if (optVal->s)
			*stringValue = strdup ( (char *) optVal->s );
		    else
			*stringValue = strdup ("");
		    stringValue2 = (char **) value;
		    if (!*stringValue2 || strcmp (*stringValue, *stringValue2))
			changed = TRUE;
		    break;
		case CompOptionTypeColor:
		    memcpy (setVal, optVal->c, sizeof (unsigned short) * 4);
		    changed |= memcmp (value, setVal,
				       sizeof (unsigned short) * 4);
		    break;
                case CompOptionTypeMatch:
		    matchValue = (CompMatch *) setVal;
		    matchInit (matchValue);
		    matchCopy (matchValue, &optVal->match);
		    changed |= matchEqual ((CompMatch *) value,
					   (CompMatch *) setVal);
		    break;
		default:
		    break;
		}
	    }
	}
	
	if (changed)
	{
	    setVal = rv + (j * structSize);
	    (*fini) (setVal, closure);
	}
	else
	    setVal = newVal;
	
	for (i = 0; i < numOptions; i++)
	{
	    value = setVal + offsets[i];
	    switch (options[i]->type)
	    {
	    case CompOptionTypeString:
		stringValue = (char **) value;
		if (*stringValue)
		    free (*stringValue);
		break;
            case CompOptionTypeMatch:
		matchValue = (CompMatch *) value;
		matchFini (matchValue);
		break;
	    default:
		break;
	    }
	}
	
	if (changed)
	{
	    memcpy (rv + (j * structSize), newVal, structSize);
	    (*init) (rv + (j * structSize), closure);
	}
	
    }

    free (options);
    free (offsets);
    free (newVal);
    *numReturn = nElements;
    return rv;
}

