# strict(<target>)
#
# Adds the project's warning and floating-point flags to one target. Warnings
# are errors: a build that passes on one machine should pass on every machine.
function(strict target)
	if(MSVC)
		target_compile_options(${target} PRIVATE
			$<$<COMPILE_LANGUAGE:CXX>:/W4 /WX /fp:strict /permissive->)
	else()
		target_compile_options(${target} PRIVATE 
            $<$<COMPILE_LANGUAGE:CXX>: 
                -Wall -Wextra -Wpedantic -Wconversion -Wswitch-enum -Werror -ffp-contract=off
            >)
		if(ENABLE_SANITIZERS)
			target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
			target_link_options(${target}    PRIVATE -fsanitize=address,undefined)
		endif()
	endif()
endfunction()
