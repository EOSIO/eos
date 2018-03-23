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
                        stash includes: 'build/**/*', name: 'buildUbuntu'
                    }
                }
                stage('MacOS') {
                    agent { label 'MacOS' }
                    steps {
                        sh '''
                            . $HOME/.bash_profile
                            echo 1 | ./eosio_build.sh 
                        ''' 
                        stash includes: 'build/**/*', name: 'buildMacOS'
                    }
                }
                stage('Fedora') {
                    agent { label 'Fedora' }
                    steps {
                        sh '''
                            . $HOME/.bash_profile
                            echo 1 | ./eosio_build.sh 
                        '''
                        stash includes: 'build/**/*', name: 'buildFedora'
                    }
                }
            }
        }
        stage('Tests') {
            parallel {
                stage('Ubuntu') {
                    agent { label 'Ubuntu' }
                    steps {
                        unstash 'buildUbuntu'
                        sh '''
                            . $HOME/.bash_profile
                            mongod -f /etc/mongod.conf > /dev/null 2>&1 &
                            cd build
                            printf "Waiting for testing to be available..."
                            while /usr/bin/pgrep -x ctest > /dev/null; do sleep 1; done
                            echo "OK!"
                            ctest --output-on-failure
                        '''
                    }
                    post {
                        failure {
                            archiveArtifacts 'build/genesis.json'
                            archiveArtifacts 'build/tn_data_00/config.ini'
                            archiveArtifacts 'build/tn_data_00/stderr.txt'
                            archiveArtifacts 'build/test_walletd_output.log'
                        }
                        always {
                            sh '''
                                mongod --shutdown
                            '''
                        }
                    }
                }
                stage('MacOS') {
                    agent { label 'MacOS' }
                    steps {
                        unstash 'buildMacOS'
                        sh '''
                            . $HOME/.bash_profile
                            mongod -f /usr/local/etc/mongod.conf > /dev/null 2>&1 &
                            cd build
                            ctest --output-on-failure
                        '''
                    }
                    post {
                        failure {
                            archiveArtifacts 'build/genesis.json'
                            archiveArtifacts 'build/tn_data_00/config.ini'
                            archiveArtifacts 'build/tn_data_00/stderr.txt'
                            archiveArtifacts 'build/test_walletd_output.log'
                        }
                        always {
                            sh '''
                                mongod --shutdown
                            '''
                        }
                    }
                }
                stage('Fedora') {
                    agent { label 'Fedora' }
                    steps {
                        unstash 'buildFedora'
                        sh '''
                            . $HOME/.bash_profile
                            mongod -f /etc/mongod.conf > /dev/null 2>&1 &
                            cd build
                            printf "Waiting for testing to be available..."
                            while /usr/bin/pgrep -x ctest > /dev/null; do sleep 1; done
                            echo "OK!"
                            ctest --output-on-failure
                        '''
                    }
                    post {
                        failure {
                            archiveArtifacts 'build/genesis.json'
                            archiveArtifacts 'build/tn_data_00/config.ini'
                            archiveArtifacts 'build/tn_data_00/stderr.txt'
                            archiveArtifacts 'build/test_walletd_output.log'
                        }
                        always {
                            sh '''
                                mongod --shutdown
                            '''
                        }
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
