add_executable(main
        main.c
        src/pico_servo.c
        src/pico_servo.h
        )

# pull in common dependencies
target_link_libraries(main pico_stdlib hardware_uart hardware_pwm hardware_adc)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(main)

