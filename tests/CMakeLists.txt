# Create the virtual environment
execute_process(
    COMMAND cmake -E env "WORKON_HOME=${CMAKE_CURRENT_BINARY_DIR}"
            pipenv install
)

# Discover tests
execute_process(
    COMMAND cmake -E env "WORKON_HOME=${CMAKE_CURRENT_BINARY_DIR}"
            pipenv run python "${CMAKE_CURRENT_SOURCE_DIR}/list_tests.py"
                unit
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE UNIT_TESTS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
separate_arguments(UNIT_TESTS UNIX_COMMAND "${UNIT_TESTS}")


# Add a test for each Python file in this directory
foreach(TEST ${UNIT_TESTS})
    add_test(
        NAME "${TEST}"
        COMMAND cmake -E env
                    "CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
                    "WORKON_HOME=${CMAKE_CURRENT_BINARY_DIR}"
                pipenv run python -m unittest -v "${TEST}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/unit"
    )
endforeach()