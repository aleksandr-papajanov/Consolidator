const { bankHasGroup } = require("./BankManagerRows.js");

const MAX_GROUP_COUNT = 16;

function deriveBankManagerActions(
    rows,
    selectedCount,
    focusedBank,
    scope,
    commitInProgress,
    refineInProgress)
{
    const groupContext = bankHasGroup(focusedBank);
    if (scope)
    {
        scope.setGroupContext(groupContext, groupContext ? focusedBank.color : null);
    }

    return {
        groupAction: {
            enabled: selectedCount >= 2 && nextBankGroupId(rows) >= 0,
            active: false
        },
        ungroupAction: {
            enabled: groupContext && Number(focusedBank.groupId) > 0,
            active: false
        },
        scopeAction: {
            enabled: groupContext,
            active: Boolean(scope && scope.isGroup()),
            color: groupContext ? focusedBank.color : null
        },
        clearAction: {
            enabled: rows.some((row) => {
                return (row.banks || []).some((bank) => {
                    return bankHasGroup(bank) && Number(bank.groupId) > 0;
                });
            })
        },
        commitAction: {
            enabled: !commitInProgress &&
                Boolean(focusedBank && focusedBank.effectActive),
            active: false
        },
        refineAction: {
            enabled: !refineInProgress &&
                Boolean(focusedBank && focusedBank.effectActive),
            active: false
        }
    };
}

function nextBankGroupId(rows)
{
    const used = {};
    (rows || []).forEach((row) => {
        (row.banks || []).forEach((bank) => {
            if (bankHasGroup(bank))
            {
                used[String(bank.groupId)] = true;
            }
        });
    });
    for (let groupId = 1; groupId < MAX_GROUP_COUNT; groupId += 1)
    {
        if (!used[String(groupId)])
        {
            return groupId;
        }
    }
    return -1;
}

module.exports = {
    deriveBankManagerActions: deriveBankManagerActions,
    nextBankGroupId: nextBankGroupId
};
