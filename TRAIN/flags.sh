###################
## Compile Flags ##
###################

CPP_STANDARD='c++17'

COMPILE_FLAGS="-std=$CPP_STANDARD -pedantic-errors -fdiagnostics-color=always -pipe"
COMPILE_FLAGS+=' -O3 -g0 -pthread'
COMPILE_FLAGS+=' -DNDEBUG'
COMPILE_FLAGS+=" -DCACHE_LINE_BYTE_SIZE=$(cat /sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size)"
COMPILE_FLAGS+=' -idirafter /PACKAGES/doctest/doctest'

# Need to recompile on every CPU!
COMPILE_FLAGS+=' -march=native -mtune=native'

COMPILE_FLAGS_GNU=''

COMPILE_FLAGS_LLVM=''
#COMPILE_FLAGS_LLVM='-stdlib=libc++'


###################
## Warning Flags ##
###################

WARNING_FLAGS='-Wpedantic -Wall -Wextra'
# Except:
WARNING_FLAGS+=' -Wno-unused-function -Wno-unused-variable'

WARNING_FLAGS_GNU='-Wshadow'

# Everything C++17.
WARNING_FLAGS_LLVM='-Weverything -Wno-c++98-compat -Wno-c++11-compat -Wno-c++14-compat'
# Except:
WARNING_FLAGS_LLVM+=' -Wno-ctad-maybe-unsupported -Wno-padded -Wno-unused-member-function -Wno-unused-template -Wno-weak-vtables'


###############
## GNU Flags ##
###############

# -flto seems to be slightly slower on an Intel i7, although it decreases executable sizes.
BUILD_FLAGS_GNU="-fno-lto $COMPILE_FLAGS $COMPILE_FLAGS_GNU $WARNING_FLAGS $WARNING_FLAGS_GNU"
LINK_FLAGS_GNU='-fno-lto -s'


################
## LLVM flags ##
################

# -flto seems to be slightly slower on an Intel i7, although it decreases executable sizes.
BUILD_FLAGS_LLVM="-fno-lto $COMPILE_FLAGS $COMPILE_FLAGS_LLVM $WARNING_FLAGS $WARNING_FLAGS_LLVM"
LINK_FLAGS_LLVM='-fno-lto -s -fuse-ld=lld'


####################
## cppcheck Flags ##
####################

CPPCHECK_FLAGS="--std=$CPP_STANDARD --enable=all"
CPPCHECK_FLAGS+=' --suppress=missingInclude --suppress=useStlAlgorithm'


######################
## clang-tidy Flags ##
######################

CLANG_TIDY_FLAGS='--quiet --use-color --header-filter=.*'
# Checks.
CLANG_TIDY_FLAGS+=' --checks='
CLANG_TIDY_FLAGS+='abseil-*'
CLANG_TIDY_FLAGS+=',android-*'
CLANG_TIDY_FLAGS+=',cert-*'
CLANG_TIDY_FLAGS+=',clang-analyzer-*'
CLANG_TIDY_FLAGS+=',concurrency-*'
CLANG_TIDY_FLAGS+=',linuxkernel-*'
CLANG_TIDY_FLAGS+=',misc-*,-misc-non-private-member-variables-in-classes'
CLANG_TIDY_FLAGS+=',modernize-*,-modernize-pass-by-value,-modernize-use-equals-delete,-modernize-use-nodiscard,-modernize-use-trailing-return-type'
CLANG_TIDY_FLAGS+=',performance-*'
CLANG_TIDY_FLAGS+=',portability-*'

# Add LLVM flags.
BUILD_FLAGS_LLVM_ARRAY=($BUILD_FLAGS_LLVM)
for BUILD_FLAG_LLVM in "${BUILD_FLAGS_LLVM_ARRAY[@]}"; do
  CLANG_TIDY_FLAGS+=" --extra-arg=$BUILD_FLAG_LLVM"
done
