# Trace API Plugin

This is a temporary document defining design decisions for the Trace API plugin's internal interfaces.  IT IS NOT INTENDED TO SURVIVE TO RELEASE.

## Expected Interfaces
### metadata log writer
1. Handle existing slice files (relaunch of nodeos)
    - identify previous slice (i.e. existing file had slice of 1000, restarting with 2000)
    - validate previous slice metadata and data log consistency
2. write metadata_log entry to index file
    - identify consistency with previous data (block log only +1 from previous block or decreases - and above lib)
    - first entry after pre-existing file needs to be in highest slice (or 1 past if highest slice has entry at the end of the slice) or less than, but not before lib
    - write LIB data to end of a previous slice when not in current slice
3. identify end of slice and new slice creation

### metadata log reader
1. identify 

### data log writer
1. File to write to (and offset)
2. Report current offset
3. Write data log to current offset 

### data log reader
1. File to read from (and offset) returns data_log 
