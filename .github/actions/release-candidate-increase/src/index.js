import * as core from "@actions/core"
import * as github from "@actions/github"

async function main() {
    const token = core.getInput('token');

    const gh = github.getOctokit(token);
    const owner = github.context.repo.owner;
    const name = github.context.repo.repo;

    const rcTagRegex = /^v?(\d+\.d+\.\d+)\-rc(\d+)$/g;

    let lastTag = null;

    const tagQuery = await gh.graphql(`
        query LatestTag($owner: String!, $name: String!) {
            repository(owner: $owner, name: $name) {
                refs(
                    refPrefix: "refs/tags/"
                    first: 1
                    orderBy: {
                        field: TAG_COMMIT_DATE
                        direction: DESC
                    }
                ) {
                    nodes {
                        name
                        target {
                            ... on Commit {
                                oid
                                committedDate
                            }
                            ... on Tag {
                                target {
                                    ... on Commit {
                                        oid
                                        committedDate
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    `, {
        owner,
        name
    });

    if (tagQuery.repository.refs.nodes.length < 1) {
        return core.setFailed("Couldn't find the latest tag. Make sure you have at least one tag created.");
    }

    lastTag = tagQuery.repository.refs.nodes[0].name;
    const matches = [...lastTag.matchAll(rcTagRegex)];
    if (matches.length < 1) {
        return core.setFailed(`Failed to determine latest RC number from version $lastTag. Is it not an RC version?`);
    }

    const nextRc = matches[2] + 1;
    const nextRcVersion = `${matches[1]}-rc${nextRc}`;

    core.setOutput('current', lastTag);
    core.setOutput('next', nextRcVersion);
    core.setOutput('nextVersion', matches[1]);
    core.setOutput('nextVersionStrict', matches[1].startsWith('v') ? matches[1].substring(1) : matches[1]);
    core.setOutput('nextReleaseCandidate', nextRc);
}

main();
