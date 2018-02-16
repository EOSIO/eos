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
    }
    post { 
        always { 
            cleanWs()
        }
    }
}