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
                            echo 1 | ./eosio_build.sh
                        '''
                    }
                }
                stage('MacOS') {
                    agent { label 'MacOS' }
                    steps {
                        sh '''
                            . $HOME/.bash_profile
                            echo 1 | ./eosio_build.sh 
                        ''' 
                    }
                }
                stage('Fedora') {
                    agent { label 'Fedora' }
                    steps {
                        sh '''
                            . $HOME/.bash_profile
                            echo 1 | ./eosio_build.sh 
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
                            cd build
                            printf "Waiting for testing to be available..."
                            while /usr/bin/pgrep -x ctest > /dev/null; do sleep 1; done
                            echo "OK!"
                            ctest --output-on-failure
                        '''
                    }
                }
                stage('Fedora') {
                    agent { label 'Fedora' }
                    steps {
                        sh '''
                            . $HOME/.bash_profile
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

            node('Fedora') {
                cleanWs()
            }
        }
    }
}
