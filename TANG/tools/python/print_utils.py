
# Log a common message
def LogCommon(msg: str):
    print(f'{msg}')


# Log a shader build error
def LogError(msg: str):
    LogCommon(f'[ ERROR ] {msg}')
    

# Log a shader build warning
def LogWarning(msg: str):
    LogCommon(f'[ WARNING ] {msg}')
    

# Log a generic info message
def LogInfo(msg: str):
    LogCommon(f'[ INFO ] {msg}')
    

# Log a build step
def LogBuildStep(msg: str):
    LogCommon(f'[ {msg.upper()} ]')