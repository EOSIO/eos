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