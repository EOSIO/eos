#!/usr/bin/env node
/* includes */
const execSync = require('child_process').execSync; // run shell commands
const fetch = require('node-fetch'); // downloading
const fs = require('fs'); // file stream
const XML = require('xml2js'); // parse xml

/* globals */
const buildkiteAccessToken = `?access_token=${process.env.BUILDKITE_API_KEY}`; // import buildkite access token from environment
const debug = (process.env.DEBUG === 'true') ? true : false;
let errorCount = 0; // count number of jobs which caused an error
const EXIT_SUCCESS = 0;
const inBuildkite = (process.env.BUILDKITE === 'true') ? true : false;
const outputFile = 'test-metrics.json';
const pipelineWhitelist = // the pipelines for which we run diagnostics
[
    'eosio',
    'eosio-base-images',
    'eosio-beta',
    'eosio-build-unpinned',
    'eosio-debug',
    'eosio-lrt',
    'eosio-security'
];

/* functions */
// given a url string, download a text document
async function download(url)
{
    if (debug) console.log(`download(${url.replace(buildkiteAccessToken, '')})`); // DEBUG
    const httpResponse = await fetch(url);
    const body = await httpResponse.text();
    if (isNullOrEmpty(body))
    {
        console.log(`ERROR: URL returned nothing! URL: ${url.replace(buildkiteAccessToken, '')}`);
        const error =
        {
            http: { body, response: httpResponse, url},
            message: 'http body is null or empty',
            origin: 'download()',
        }
        throw error;
    }
    if (debug) console.log('Download complete.'); // DEBUG
    return body;
}

// given a pipeline and a build number, get a build object
async function getBuild(pipeline, buildNumber)
{
    if (debug) console.log(`getBuild(${pipeline}, ${buildNumber})`); // DEBUG
    const httpResponse = await fetch(`https://api.buildkite.com/v2/organizations/EOSIO/pipelines/${pipeline}/builds/${buildNumber}${buildkiteAccessToken}`);
    return httpResponse.json();
}

// given a buildkite job, return the environmental variables
async function getEnvironment(job)
{
    if (debug) console.log('getEnvironment()'); // DEBUG
    const httpResponse = await fetch(`${job.build_url}/jobs/${job.id}/env${buildkiteAccessToken}`);
    const environment = await httpResponse.json();
    return environment.env;
}

// given a string to search, a key as regex or a string, and optionally a start index, return the lowest line number containing the key
function getLineNumber(text, key, startIndex)
{
    if (debug) console.log('getLineNumber()'); // DEBUG
    const begin = (isNullOrEmpty(startIndex) || !Number.isInteger(startIndex) || startIndex < 1) ? 0 : startIndex;
    let found = false;
    let lineNumber = 0;
    const regex = (key instanceof RegExp);
    text.split('\n').some((line) =>
    {
        if (lineNumber >= begin && ((regex && key.test(line)) || (!regex && line.includes(key))))
        {
            found = true;
            return true; // c-style break
        }
        lineNumber += 1;
        return false; // for the linter, plz delete when linter is fixed
    });
    return (found) ? lineNumber : -1;
}

// given a buildkite job, return a sanitized log file
async function getLog(job)
{
    if (debug) console.log(`getLog(${job.raw_log_url})`); // DEBUG
    const logText = await download(job.raw_log_url + buildkiteAccessToken);
    // returns log lowercase, with single spaces and '\n' only, and only ascii-printable characters
    return sanitize(logText); // made this a separate function for unit testing purposes
}

// given a Buildkite environment, return the operating system used
function getOS(environment)
{
    if (debug) console.log(`getOS(${environment.BUILDKITE_LABEL})`); // DEBUG
    if (isNullOrEmpty(environment) || isNullOrEmpty(environment.BUILDKITE_LABEL))
    {
        console.log('ERROR: getOS() called with empty environment.BUILDKITE_LABEL!');
        console.log(JSON.stringify(environment));
        return null;
    }
    const label = environment.BUILDKITE_LABEL.toLowerCase();
    if ((/aws(?!.*[23])/.test(label) || /amazon(?!.*[23])/.test(label)))
        return 'Amazon Linux 1';
    if (/aws.*2/.test(label) || /amazon.*2/.test(label))
        return 'Amazon Linux 2';
    if (/centos(?!.*[89])/.test(label))
        return 'CentOS 7';
    if (/fedora(?!.*2[89])/.test(label) && /fedora(?!.*3\d)/.test(label))
        return 'Fedora 27';
    if (/high.*sierra/.test(label))
        return 'High Sierra';
    if (/mojave/.test(label))
        return 'Mojave';
    if (/ubuntu.*16.*04/.test(label) || /ubuntu.*16(?!.*10)/.test(label))
        return 'Ubuntu 16.04';
    if (/ubuntu.*18.*04/.test(label) || /ubuntu.*18(?!.*10)/.test(label))
        return 'Ubuntu 18.04';
    if (/docker/.test(label))
        return 'Docker';
    return 'Unknown';
}

// given a Buildkite job, return the test-results.xml file as JSON
async function getXML(job)
{
    if (debug) console.log('getXML()'); // DEBUG
    const xmlFilename = 'test-results.xml';
    const artifacts = await download(job.artifacts_url + buildkiteAccessToken);
    const testResultsArtifact = JSON.parse(artifacts).filter(artifact => artifact.filename === xmlFilename);
    if (isNullOrEmpty(testResultsArtifact))
    {
        console.log(`WARNING: No ${xmlFilename} found for "${job.name}"! Link: ${job.web_url}`);
        return null;
    }
    const urlBuildkite = testResultsArtifact[0].download_url;
    const rawXML = await download(urlBuildkite + buildkiteAccessToken);
    const xmlOptions =
    {
        attrNameProcessors: [function lower(name) { return name.toLowerCase(); }],
        explicitArray: false, // do not put single strings in single-element arrays
        mergeAttrs: true, // make attributes children of their node
        normalizeTags: true, // convert all tag names to lowercase
    };
    let xmlError, xmlTestResults;
    await XML.parseString(rawXML, xmlOptions, (err, result) => {xmlTestResults = result; xmlError = err;});
    if (isNullOrEmpty(xmlError))
        return xmlTestResults;
    console.log(`WARNING: Failed to parse xml for "${job.name}" job! Link: ${job.web_url}`);
    console.log(JSON.stringify(xmlError));
    return null;
}

// test if variable is empty
function isNullOrEmpty(str)
{
    return (str === null || str === undefined || str.length === 0 || /^\s*$/.test(str));
}

// return array of test results from a buildkite job log
function parseLog(logText)
{
    if (debug) console.log('parseLog()'); // DEBUG
    const lines = logText.split('\n');
    const resultLines = lines.filter(line => /test\s+#\d+/.test(line)); // 'grep' for the test result lines
    // parse the strings and make test records
    return resultLines.map((line) =>
    {
        const y = line.trim().split(/test\s+#\d+/).pop(); // remove everything before the test declaration
        const parts = y.split(/\s+/).slice(1, -1); // split the line and remove the test number and time unit
        const testName = parts[0];
        const testTime = parts[(parts.length - 1)];
        const rawResult = parts.slice(1, -1).join();
        let testResult;
        if (rawResult.includes('failed'))
            testResult = 'Failed';
        else if (rawResult.includes('passed'))
            testResult = 'Passed';
        else
            testResult = 'Exception';
        return { testName, testResult, testTime }; // create a test record
    });
}

// return array of test results from an xUnit-formatted JSON object
function parseXunit(xUnit)
{
    if (debug) console.log('parseXunit()'); // DEBUG
    if (isNullOrEmpty(xUnit))
    {
        console.log('WARNING: xUnit is empty!');
        return null;
    }
    return xUnit.site.testing.test.map((test) =>
    {
        const testName = test.name;
        const testTime = test.results.namedmeasurement.filter(x => /execution\s+time/.test(x.name.toLowerCase()))[0].value;
        let testResult;
        if (test.status.includes('failed'))
            testResult = 'Failed';
        else if (test.status.includes('passed'))
            testResult = 'Passed';
        else
            testResult = 'Exception';
        return { testName, testResult, testTime };
    });
}

// returns text lowercase, with single spaces and '\n' only, and only ascii-printable characters
function sanitize(text)
{
    if (debug) console.log(`sanitize(text) where text.length = ${text.length} bytes`); // DEBUG
    const chunkSize = 131072; // process text in 128 kB chunks
    if (text.length > chunkSize)
        return sanitize(text.slice(0, chunkSize)).concat(sanitize(text.slice(chunkSize)));
    return text
        .replace(/(?!\n)\r(?!\n)/g, '\n').replace(/\r/g, '') // convert all line endings to '\n'
        .replace(/[^\S\n]+/g, ' ') // convert all whitespace to ' '
        .replace(/[^ -~\n]+/g, '') // remove non-printable characters
        .toLowerCase();
}

// input is array of whole lines containing "test #" and ("failed" or "exception")
function testDiagnostics(test, logText)
{
    if (debug)
    {
        console.log(`testDiagnostics(test, logText) where logText.length = ${logText.length} bytes and test is`); // DEBUG
        console.log(JSON.stringify(test));
    }
    // get basic information
    const testResultLine = new RegExp(`test\\s+#\\d+.*${test.testName}`, 'g'); // regex defining "test #" line
    const startIndex = getLineNumber(logText, testResultLine);
    const output = { errorMsg: null, lineNumber: startIndex + 1, stackTrace: null }; // default output
    // filter tests
    if (test.testResult.toLowerCase() === 'passed')
        return output;
    output.errorMsg = 'test diangostics are not enabled for this pipeline';
    if (!pipelineWhitelist.includes(test.pipeline))
        return output;
    // diagnostics
    if (debug) console.log('Running diagnostics...'); // DEBUG
    output.errorMsg = 'uncategorized';
    const testLog = logText.split(testResultLine)[1].split(/test\s*#/)[0].split('\n'); // get log output from this test only, as array of lines
    let errorLine = testLog[0]; // first line, from "test ## name" to '\n' exclusive
    if (/\.+ *\** *not run\s+0+\.0+ sec$/.test(errorLine)) // not run
        output.errorMsg = 'test not run';
    else if (/\.+ *\** *time *out\s+\d+\.\d+ sec$/.test(errorLine)) // timeout
        output.errorMsg = 'test timeout';
    else if (/exception/.test(errorLine)) // test exception
        output.errorMsg = errorLine.split('exception')[1].replace(/[: \d.]/g, '').replace(/sec$/, ''); // isolate the error message after exception
    else if (/fc::.*exception/.test(testLog.filter(line => !isNullOrEmpty(line))[1])) // fc exception
    {
        [, errorLine] = testLog.filter(line => !isNullOrEmpty(line)); // get first line
        output.errorMsg = `fc::${errorLine.split('::')[1].replace(/['",]/g, '').split(' ')[0]}`; // isolate fx exception body
    }
    else if (testLog.join('\n').includes('ctest:')) // ctest exception
    {
        [errorLine] = testLog.filter(line => line.includes('ctest:'));
        output.errorMsg = `ctest:${errorLine.split('ctest:')[1]}`;
    }
    else if (!isNullOrEmpty(testLog.filter(line => /boost.+exception/.test(line)))) // boost exception
    {
        [errorLine] = testLog.filter(line => /boost.+exception/.test(line));
        output.errorMsg = `boost: ${errorLine.replace(/[()]/g, '').split(/: (.+)/)[1]}`; // capturing parenthesis, split only at first ' :'
        output.stackTrace = testLog.filter(line => /thread-\d+/.test(line))[0].split('thread-')[1].replace(/^\d+/, '').trim().replace(/[[]\d+m$/, ''); // get the bottom of the stack trace
    }
    else if (/unit[-_. ]+test/.test(test.testName) || /plugin[-_. ]+test/.test(test.testName)) // unit test, application exception
    {
        if (!isNullOrEmpty(testLog.filter(line => line.includes('exception: '))))
        {
            [errorLine] = testLog.filter(line => line.includes('exception: '));
            [, output.errorMsg] = errorLine.replace(/[()]/g, '').split(/: (.+)/); // capturing parenthesis, split only at first ' :'
            output.stackTrace = testLog.filter(line => /thread-\d+/.test(line))[0].split('thread-')[1].replace(/^\d+/, '').trim().replace(/[[]\d+m$/, ''); // get the bottom of the stack trace
        }
        // else uncategorized unit test
    }
    // else integration test, add cross-referencing code here (or uncategorized)
    if (errorLine !== testLog[0]) // get real line number from log file
        output.lineNumber = getLineNumber(logText, errorLine, startIndex) + 1;
    return output;
}

// return test metrics given a buildkite job or build
async function testMetrics(buildkiteObject)
{
    if (!isNullOrEmpty(buildkiteObject.type)) // input is a Buildkite job object
    {
        const job = buildkiteObject;
        console.log(`Processing test metrics for "${job.name}"${(inBuildkite) ? '' : ` at ${job.web_url}`}...`);
        if (isNullOrEmpty(job.exit_status))
        {
            console.log(`${(inBuildkite) ? '+++ :warning: ' : ''}WARNING: "${job.name}" was skipped!`);
            return null;
        }
        // get test results
        const logText = await getLog(job);
        let testResults;
        let xUnit;
        try
        {
            xUnit = await getXML(job);
            testResults = parseXunit(xUnit);
        }
        catch (error)
        {
            console.log(`XML processing failed for "${job.name}"! Link: ${job.web_url}`);
            console.log(JSON.stringify(error));
            testResults = null;
        }
        finally
        {
            if (isNullOrEmpty(testResults))
                testResults = parseLog(logText);
        }
        // get test metrics
        const env = await getEnvironment(job);
        env.BUILDKITE_REPO = env.BUILDKITE_REPO.replace(new RegExp('^git@github.com:(EOSIO/)?'), '').replace(new RegExp('.git$'), '');
        const metrics = [];
        const os = getOS(env);
        testResults.forEach((result) =>
        {
            // add test properties
            const test =
            {
                ...result, // add testName, testResult, testTime
                agentName: env.BUILDKITE_AGENT_NAME,
                agentRole: env.BUILDKITE_AGENT_META_DATA_QUEUE || env.BUILDKITE_AGENT_META_DATA_ROLE,
                branch: env.BUILDKITE_BRANCH,
                buildNumber: env.BUILDKITE_BUILD_NUMBER,
                commit: env.BUILDKITE_COMMIT,
                job: env.BUILDKITE_LABEL,
                os,
                pipeline: env.BUILDKITE_PIPELINE_SLUG,
                repo: env.BUILDKITE_REPO,
                testTime: parseFloat(result.testTime),
                url: job.web_url,
            };
            metrics.push({ ...test, ...testDiagnostics(test, logText) });
        });
        return metrics;
    }
    else if (!isNullOrEmpty(buildkiteObject.number)) // input is a Buildkite build object
    {
        const build = buildkiteObject;
        console.log(`Processing test metrics for ${build.pipeline.slug} build ${build.number}${(inBuildkite) ? '' : ` at ${build.web_url}`}...`);
        let metrics = [], promises = [];
        // process test metrics
        build.jobs.filter(job => job.type === 'script' && /test/.test(job.name.toLowerCase()) && ! /test metrics/.test(job.name.toLowerCase())).forEach((job) =>
        {
            promises.push(
                testMetrics(job)
                    .then((moreMetrics) => {
                        if (!isNullOrEmpty(moreMetrics))
                            metrics = metrics.concat(moreMetrics);
                        else
                            console.log(`${(inBuildkite) ? '+++ :warning: ' : ''}WARNING: "${job.name}" metrics are empty!\nmetrics = ${JSON.stringify(moreMetrics)}`);
                    }).catch((error) => {
                        console.log(`${(inBuildkite) ? '+++ :no_entry: ' : ''}ERROR: Failed to process test metrics for "${job.name}"! Link: ${job.web_url}`);
                        console.log(JSON.stringify(error));
                        errorCount++;
                    })
            );
        });
        await Promise.all(promises);
        return metrics;
    }
    else // something else
    {
        console.log(`${(inBuildkite) ? '+++ :no_entry: ' : ''}ERROR: Buildkite object not recognized or not a test step!`);
        console.log(JSON.stringify({buildkiteObject}));
        return null;
    }
}

/* main */
async function main()
{
    if (debug) console.log(`$ ${process.argv.join(' ')}`);
    let build, metrics = null;
    console.log(`${(inBuildkite) ? '+++ :evergreen_tree: ' : ''}Getting information from enviroment...`);
    const buildNumber = process.env.BUILDKITE_BUILD_NUMBER || process.argv[2];
    const pipeline = process.env.BUILDKITE_PIPELINE_SLUG || process.argv[3];
    if (debug)
    {
        console.log(`BUILDKITE=${process.env.BUILDKITE}`);
        console.log(`BUILDKITE_BUILD_NUMBER=${process.env.BUILDKITE_BUILD_NUMBER}`);
        console.log(`BUILDKITE_PIPELINE_SLUG=${process.env.BUILDKITE_PIPELINE_SLUG}`);
        console.log('    State:')
        console.log(`inBuildkite = "${inBuildkite}"`);
        console.log(`buildNumber = "${buildNumber}"`);
        console.log(`pipeline    = "${pipeline}"`);
    }
    if (isNullOrEmpty(buildNumber) || isNullOrEmpty(pipeline) || isNullOrEmpty(process.env.BUILDKITE_API_KEY))
    {
        console.log(`${(inBuildkite) ? '+++ :no_entry: ' : ''}ERROR: Missing required inputs!`);
        if (isNullOrEmpty(process.env.BUILDKITE_API_KEY)) console.log('- Buildkite API key, as BUILDKITE_API_KEY environment variable');
        if (isNullOrEmpty(buildNumber)) console.log('- Build Number, as BUILDKITE_BUILD_NUMBER or argument 1');
        if (isNullOrEmpty(pipeline)) console.log('- Pipeline Slug, as BUILDKITE_PIPELINE_SLUG or argument 2');
        errorCount = -1;
    }
    else
    {
        console.log(`${(inBuildkite) ? '+++ :bar_chart: ' : ''}Processing test metrics...`);
        build = await getBuild(pipeline, buildNumber);
        metrics = await testMetrics(build);
        console.log('Done processing test metrics.');
    }
    console.log(`${(inBuildkite) ? '+++ :pencil: ' : ''}Writing to file...`);
    fs.writeFileSync(outputFile, JSON.stringify({ metrics }));
    console.log(`Saved metrics to "${outputFile}" in "${process.cwd()}".`);
    if (inBuildkite)
    {
        console.log('+++ :arrow_up: Uploading artifact...');
        execSync(`buildkite-agent artifact upload ${outputFile}`);
    }
    if (errorCount === 0)
        console.log(`${(inBuildkite) ? '+++ :white_check_mark: ' : ''}Done!`);
    else
    {
        console.log(`${(inBuildkite) ? '+++ :warning: ' : ''}Finished with errors.`);
        console.log(`Please send automation a link to this job${(isNullOrEmpty(build)) ? '.' : `: ${build.web_url}`}`);
        console.log('@kj4ezj or @zreyn on Telegram');
    }
    return (inBuildkite) ? process.exit(EXIT_SUCCESS) : process.exit(errorCount);
};

main();