function set_system_vars() {
    if [[ $ARCH == "Darwin" ]]; then
        export OS_VER=$(sw_vers -productVersion)
        export OS_MAJ=$(echo "${OS_VER}" | cut -d'.' -f1)
        export OS_MIN=$(echo "${OS_VER}" | cut -d'.' -f2)
        export OS_PATCH=$(echo "${OS_VER}" | cut -d'.' -f3)
        export MEM_GIG=$(bc <<< "($(sysctl -in hw.memsize) / 1024000000)")
        export DISK_INSTALL=$(df -h . | tail -1 | tr -s ' ' | cut -d\  -f1 || cut -d' ' -f1)
        export blksize=$(df . | head -1 | awk '{print $2}' | cut -d- -f1)
        export gbfactor=$(( 1073741824 / blksize ))
        export total_blks=$(df . | tail -1 | awk '{print $2}')
        export avail_blks=$(df . | tail -1 | awk '{print $4}')
        export DISK_TOTAL=$((total_blks / gbfactor ))
        export DISK_AVAIL=$((avail_blks / gbfactor ))
    else
        export DISK_INSTALL=$( df -h . | tail -1 | tr -s ' ' | cut -d\  -f1 )
        export DISK_TOTAL_KB=$( df . | tail -1 | awk '{print $2}' )
        export DISK_AVAIL_KB=$( df . | tail -1 | awk '{print $4}' )
        export MEM_GIG=$(( ( ( $(cat /proc/meminfo | grep MemTotal | awk '{print $2}') / 1000 ) / 1000 ) ))
        export DISK_TOTAL=$(( DISK_TOTAL_KB / 1048576 ))
        export DISK_AVAIL=$(( DISK_AVAIL_KB / 1048576 ))
    fi
    export JOBS=$(( MEM_GIG > CPU_CORES ? CPU_CORES : MEM_GIG ))
}