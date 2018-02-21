pipeline {
    agent any
    stages {
        stage('Build') {
            parallel {
                stage('Ubuntu') {
                    steps {
                        sh '''
                            . $HOME/.bash_profile
                            ./eosio_build.sh
                        '''
                    }
                }
                stage('MacOS') {
                    steps {
                        sh '''
                            . $HOME/.bash_profile
                            echo "Darwin build coming soon..."
                        ''' 
                    }
                }
                stage('Fedora') {
                    steps {
                        sh '''
                            . $HOME/.bash_profile
                            echo "Fedora build coming soon..."
                        ''' 
                    }
                }
            }
        }
        stage('Tests') {
            parallel {
                stage('Ubuntu') {
                    steps {
                        sh '''
                            . $HOME/.bash_profile
                            export EOSLIB=$(pwd)/contracts
                            cd build
                            printf "Waiting for testing to be available..."
                            while /usr/bin/pgrep -x ctest > /dev/null; do sleep 1; done
                            echo "OK!"
                            ctest --output-on-failure
                        '''
                    }
                }
                stage('MacOS') {
                    steps {
                        sh '''
                            . $HOME/.bash_profile
                            echo "Darwin tests coming soon..."
                        '''
                    }
                }
                stage('Fedora') {
                    steps {
                        sh '''
                            . $HOME/.bash_profile
                            echo "Fedora tests coming soon..."
                        '''
                    }
                }
            }
        }
    }
    post { 
        always { 
            cleanWs()
        }
    }
}