pipeline {
    agent none
    stages {
        stage('Build') {
            parallel {
                stage('Ubuntu') {
                    agent { label 'Ubuntu' }
                    steps {
                        sh '''
                            . $HOME/.bash_profile
                            ./eosio_build.sh
                        '''
                    }
                }
                stage('MacOS') {
                    agent { label 'MacOS' }
                    steps {
                        sh '''
                            . $HOME/.bash_profile
                            ./eosio_build.sh 
                        ''' 
                    }
                }
            }
        }
        stage('Tests') {
            parallel {
                stage('Ubuntu') {
                    agent { label 'Ubuntu' }
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
                    agent { label 'MacOS' }
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
            }
        }
    }
    post { 
        always { 
            node('Ubuntu') {
                cleanWs()
            }
            
            node('MacOS') {
                cleanWs()
            }
        }
    }
}